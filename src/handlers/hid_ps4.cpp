/**
 * @file hid_ps4.cpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2024-02-11
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "default_hid_handler.hpp"
#include "hid_handler_builder.hpp"

#include "pico/stdlib.h"
#include "tusb.h"

#include "controller_port.hpp"

// a lot is stolen from here as the tinyusb example already supports DS4
// https://github.com/hathach/tinyusb/blob/master/examples/host/hid_controller/src/hid_app.c

/// Sony DS4 report layout detail https://www.psdevwiki.com/ps4/DS4-USB
struct __attribute__((packed)) Report {
    uint8_t report_id;
    uint8_t joy_left_x, joy_left_y, z, rz; // joystick

    struct {
        uint8_t dpad : 4;     // (hat format, 0x08 is released, 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW)
        uint8_t square : 1;   // west
        uint8_t cross : 1;    // south
        uint8_t circle : 1;   // east
        uint8_t triangle : 1; // north
    };

    struct {
        uint8_t l1 : 1;
        uint8_t r1 : 1;
        uint8_t l2 : 1;
        uint8_t r2 : 1;
        uint8_t share : 1;
        uint8_t option : 1;
        uint8_t l3 : 1;
        uint8_t r3 : 1;
    };

    struct {
        uint8_t ps : 1;      // playstation button
        uint8_t tpad : 1;    // track pad click
        uint8_t counter : 6; // +1 each report
    };

    uint8_t l2_trigger; // 0 released, 0xff fully pressed
    uint8_t r2_trigger; // as above

    //  uint16_t timestamp;
    //  uint8_t  battery;
    //
    //  int16_t gyro[3];  // x, y, z;
    //  int16_t accel[3]; // x, y, z

    // there is still lots more info
};

/**
 * @brief Driver for the PS4 Dual Shock Controller
 *
 */
class PS4DualShockHandler : public DefaultHidHandler {

  private:
    /// @brief Neutral position of a joystick axis.
    static constexpr uint32_t kAnalogCenter{0x80};

    /// @brief Minimum difference to center to register a direction
    static constexpr uint32_t kAnalogThreshold{0x40};

    /**
     * @brief Converts dpad value of a ps4 controller to 4 single buttons
     * This should not to be confused with coolie hat values which are
     * using different values
     *
     * @param report output struct to store data in
     * @param hat_dir 4 bit input
     */
    void update_from_dpad(GamepadReport &report, uint8_t hat_dir) {
        report.up = (hat_dir == 0 || hat_dir == 1 || hat_dir == 7);
        report.right = (hat_dir == 2 || hat_dir == 3 || hat_dir == 1);
        report.down = (hat_dir == 4 || hat_dir == 5 || hat_dir == 3);
        report.left = (hat_dir == 6 || hat_dir == 7 || hat_dir == 5);
    }

  public:
    void process_report(std::span<const uint8_t> d) override {
        auto dat = reinterpret_cast<const Report *>(d.data());

        if (dat->report_id == 1) {
#if 0
            PRINTF("PS4: %d %d%d%d%d %d%d%d%d ", dat->dpad, dat->triangle, dat->circle, dat->cross, dat->square,
                   dat->l1, dat->l2, dat->r1, dat->r2);

            /*
            for (uint8_t i : d) {
                PRINTF(" %02x", i);
            }
            */
            PRINTF("\n");
#endif

            GamepadReport aj;
            update_from_dpad(aj, dat->dpad);

            aj.left |= dat->joy_left_x < (kAnalogCenter - kAnalogThreshold);
            aj.down |= dat->joy_left_y > (kAnalogCenter + kAnalogThreshold);
            aj.right |= dat->joy_left_x > (kAnalogCenter + kAnalogThreshold);
            aj.up |= dat->joy_left_y < (kAnalogCenter - kAnalogThreshold);

            aj.fire = dat->square || dat->circle;
            aj.sec_fire = dat->cross;
            aj.auto_fire = dat->triangle;

            aj.joystick_swap = dat->share;

            if (target_) {
                target_->process_gamepad_report(aj);
            }
        }
    }

    ReportType expected_report() override {
        return kGamePad;
    }
};

// https://github.com/felis/USB_Host_Shield_2.0/blob/master/PS4USB.h

#define PS4_VID 0x054C      ///< Sony Corporation
#define PS4_PID 0x05C4      ///< PS4 Controller
#define PS4_PID_SLIM 0x09CC ///< PS4 Slim Controller

static HidHandlerBuilder
    builder_normal(PS4_VID, PS4_PID, []() { return std::make_unique<PS4DualShockHandler>(); }, nullptr);
static HidHandlerBuilder
    builder_slim(PS4_VID, PS4_PID_SLIM, []() { return std::make_unique<PS4DualShockHandler>(); }, nullptr);