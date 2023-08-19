/**
 * @file mouse_c1351.hpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include "processors/interfaces.hpp"

#include "c1351.pio.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"

/**
 * @brief Emulation of the Commodore 1351 in proportional mode.
 * Uses the POTX and POTY pins to simulate analog values for the ADC inside
 * the SID.
 *
 * http://sensi.org/~svo/%5Bm%5Douse/ was a great help here
 *
 * The measurement cycle of the SID takes 512 microseconds.
 * The first 256 are spent inside the SID by draining a capacitor.
 * The second 256 are used to wait until the capacitor is charged to a certain
 * level by an external resistor. This means that the ADC is not measuring
 * voltage but resistance and it does this nearly linear which is good for
 * paddles.
 *
 * Our goal here is wait for a defined time and charge the capacitor in a timed
 * manner to simulate a certain value in the SID that we want.
 *
 * We does this by using the PIO units of the RP2040. One PIO has 4 state
 * machines. We will use all of them to drive 2x POTX and 2x POTY
 *
 */
class C1351Converter : public RunnableMouseReportProcessor {
  private:
    /// @brief Single PIO unit for all 4 required pins.
    static PIO pio_;

    /// @brief state machine to drive POTX
    int sm_x_{-1};
    /// @brief state machine to drive POTY
    int sm_y_{-1};

    /// @brief Position of program in PIO instruction memory
    static uint offset_;

    /// @brief horizontal movement still to perform using PIO
    int32_t mouse_accumulator_x{0};
    /// @brief vertical movement still to perform using PIO
    int32_t mouse_accumulator_y{0};

    /// @brief current POTX value we want to achieve
    uint32_t value_pot_x_{30};
    /// @brief current POTY value we want to achieve
    uint32_t value_pot_y_{30};

    /// @brief current mouse button state
    ControllerPortState state_;
    /// @brief last mouse button state. used to check for changes
    ControllerPortState last_state_;

    /// @brief number of microseconds the SID will drain the capacitor
    static constexpr int32_t kDrainLength = 256;
    /// @brief number of PIO clock ticks per microsecond
    static constexpr int32_t kDigitPerUs = 125;

    /// @brief clock ticks to add to achieve the most stable timing for a value
    /// of 64 on POTY
    float calibration_pot_y_64 = -10 * kDigitPerUs - 190 + 62;
    /// @brief clock ticks to add to achieve the most stable timing for a value
    /// of 191 on POTY
    float calibration_pot_y_191 = -10 * kDigitPerUs + 50 + 62;

    /// @brief clock ticks to add to achieve the most stable timing for a value
    /// of 64 on POTX
    float calibration_pot_x_64 = -10 * kDigitPerUs - 190 - 62;
    /// @brief clock ticks to add to achieve the most stable timing for a value
    /// of 191 on POTX
    float calibration_pot_x_191 = -10 * kDigitPerUs + 50 - 62;

    /**
     * @brief Checks if the PIO state machines can take another value.
     *
     * Usually the FIFOs are allowed to take up to 4 values but we don't really
     * want any latency here. Only push a value if the FIFOs are both empty.
     *
     * @return true Values can be pushed
     * @return false Wait another round
     */
    bool pio_fifos_can_take_values() {
        return pio_sm_is_tx_fifo_empty(pio_, sm_y_) &&
               pio_sm_is_tx_fifo_empty(pio_, sm_x_);
    }

    /**
     * @brief Provide PIO with drain durations to simulate a resistor
     *
     * Allowed values for C1351 are 64 to 191 according to VICE emulator.
     * This would mean that a range of 128 is available which seems to be not
     * the truth. According to https://www.c64-wiki.de/wiki/Mouse_1351, the
     * highest bit must be ignored and the lowest is considered to be a noise
     * value to help with oscillating values. This leaves us with a real range
     * of 64 values.
     *
     * @param sm            State machine to affect
     * @param pot_value     Value in range of 64 to inclusive 191
     */
    void push_calibrated_value(int sm, uint32_t pot_value) {
        float x0, y0, x1, y1, xp, yp;

        switch (sm) {
        case 1:
            y0 = calibration_pot_y_64;
            y1 = calibration_pot_y_191;
            break;
        case 0:
        default:
            y0 = calibration_pot_x_64;
            y1 = calibration_pot_x_191;
            break;
        }

        xp = static_cast<float>(pot_value);
        x0 = 64;
        x1 = 191;
        yp = y0 + ((y1 - y0) / (x1 - x0)) * (xp - x0);

        int32_t calibration = static_cast<int32_t>(yp);

        pio_sm_put(pio_, sm,
                   (kDrainLength + pot_value) * kDigitPerUs + calibration);
    }

    /// @brief  data sink for mouse button presses
    /// Also used to gather POTX and POTY pin numbers to configure the PIO
    /// machine
    std::shared_ptr<ControllerPortInterface> target_;

    /**
     * @brief Implements fractional movement speed.
     * Incremented every time a new value is pushed to the PIO which is done
     * every 512us.
     * But the C64 is unable to process much movement as it seems, so we use
     * this value to allow faster movement for some time to have fractional
     * speeds without floating point calculations. Look at the code of \ref run
     * for an example.
     *
     */
    uint32_t values_pushed_cnt_{0};

  public:
    C1351Converter() { PRINTF("C1351Converter +\n"); }
    virtual ~C1351Converter() { PRINTF("C1351Converter -\n"); }

    /**
     * @brief Registers a data sink for mouse button clicks.
     *
     * Keep in mind that the movement is not done using GPIO but PIO
     * and all that is not handled in the implementations of \ref
     * ControllerPortInterface
     *
     * @param t     implementation of a controller port
     */
    void set_target(std::shared_ptr<ControllerPortInterface> t) { target_ = t; }

    void ensure_mouse_muxing() override {
        if (target_->get_pot_y_sense_gpio() == 8) {
            sm_x_ = 0;
            sm_y_ = 1;
        } else {
            sm_x_ = 2;
            sm_y_ = 3;
        }
        pio_sm_set_enabled(pio_, sm_x_, false);
        pio_sm_set_enabled(pio_, sm_y_, false);

        sid_adc_stim_program_init(pio_, sm_x_, offset_,
                                  target_->get_pot_y_sense_gpio(),
                                  target_->get_pot_x_drain_gpio());
        sid_adc_stim_program_init(pio_, sm_y_, offset_,
                                  target_->get_pot_y_sense_gpio(),
                                  target_->get_pot_y_drain_gpio());

        PRINTF("Enable C1351 for %s port\n", target_->get_name());
    }

    /**
     * @brief Initialize PIO hardware.
     *
     * Must be called once before using the PIO.
     */
    static void setup_pio() {
        pio_ = reinterpret_cast<PIO>(PIO0_BASE);
        offset_ = pio_add_program(pio_, &sid_adc_stim_program);
    }

    void process_mouse_report(MouseReport &mouse_report) override {

        mouse_accumulator_x += mouse_report.relx;
        mouse_accumulator_y -= mouse_report.rely;

        state_.fire1 = mouse_report.left;
        state_.up = mouse_report.right;

        if (target_ && last_state_ != state_) {
            last_state_ = state_;
            target_->set_port_state(state_);
        }
    }

    /**
     * @brief Provides new pot values for the C1351 emulation.
     *
     * The SID has a measuring cycle of 512 microsecond for the
     * POT X and Y pins. The range of values is 64 as explained in \ref
     * C1351Converter::push_calibrated_value. This would mean that it should be
     * possible to have a diff of 30 between measurement cycles. This is not the
     * case at least with "THE FINAL CARTRIDGE III" as the reading frequency of
     * POT X and Y from the software is much lower than what the SID can do.
     *
     * I tried to solve this here by limiting the amount of change for every
     * measuring cycle to 2. This would mean that the change is limited to 40
     * per frame, assuming that a frame takes 20 milliseconds.
     * Without the noise bit this would mean the change is limited to 20 per
     * frame. This is weird because I would assume that 30 must be possible as
     * well, but 3 is too much for "DESKTOP V1.0" of "THE FINAL CARTRIDGE III".
     *
     * Then let's go for a fraction here. 2 times out of 8 it is 3, else 2.
     * This limits it to 2.25 which seems to be fine with "DESKTOP V1.0"
     * But not for Lemmings. I've performed measurements and the fastest
     * speed is 1.5 so I'm going for that until I find another incompatible
     * software.
     */
    void run() override {
        if (pio_fifos_can_take_values()) {
            int32_t kMaxChange =
                (values_pushed_cnt_ % 8) > 3 ? 2 : 1; // 1.5 per cycle

            values_pushed_cnt_++;

            int32_t inc_x = std::max(-kMaxChange,
                                     std::min(mouse_accumulator_x, kMaxChange));
            int32_t inc_y = std::max(-kMaxChange,
                                     std::min(mouse_accumulator_y, kMaxChange));

            mouse_accumulator_x -= inc_x;
            mouse_accumulator_y -= inc_y;
            value_pot_x_ += inc_x;
            value_pot_y_ += inc_y;

            push_calibrated_value(sm_x_, (value_pot_x_ % 128) + 64);
            push_calibrated_value(sm_y_, (value_pot_y_ % 128) + 64);
        }
    }
};
