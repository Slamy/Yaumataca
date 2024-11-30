/**
 * @file hid_impact.cpp
 * @author André Zeps, with help from Spacepilot3000
 * @brief
 * @version 0.1
 * @date 2024-11-30
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "default_hid_handler.hpp"
#include "hid_handler_builder.hpp"

#include "controller_port.hpp"
#include "pico/stdlib.h"

/**
 * @brief Packed struct representing the HID report
 */
struct __attribute__((packed)) Report {
    uint8_t report_id;

    uint8_t reserved1; // always 0x7f?
    uint8_t reserved2; // always 0x7f?
    uint8_t joy_rel_x; // center at 0x7f, left 0x00, right 0xff
    uint8_t joy_rel_y; // center at 0x7f, up 0x00, down 0xff

    // Byte 5
    uint8_t reserved : 4; // always 0xf for some reason
    uint8_t button_a : 1;
    uint8_t button_b : 1;
    uint8_t button_x : 1;
    uint8_t button_y : 1;

    // Byte 6
    uint8_t trigger_l : 1;
    uint8_t trigger_r : 1;
    uint8_t reserved3 : 2;
    uint8_t button_select : 1;
    uint8_t button_start : 1;
};

/**
 * @brief Driver for the noname impact Controller
 * Is a very early cheap USB clone of the Playstation 1 controller.
 */
class HizueHidHandler : public DefaultHidHandler {
  private:
    /// @brief Neutral position of a joystick axis.
    static constexpr uint32_t kAnalogCenter{0x80};

    /// @brief Minimum difference to center to register a direction
    static constexpr uint32_t kAnalogThreshold{0x40};

  public:
    void process_report(std::span<const uint8_t> d) {

        auto dat = reinterpret_cast<const Report *>(d.data());

#if 0
        PRINTF("DragonInc: ");
        for (uint8_t i : d) {
            PRINTF(" %02x", i);
        }
        PRINTF("\n");
#endif

        GamepadReport aj;

        aj.left = dat->joy_rel_x < (kAnalogCenter - kAnalogThreshold);
        aj.down = (dat->joy_rel_y > (kAnalogCenter + kAnalogThreshold));
        aj.right = (dat->joy_rel_x > (kAnalogCenter + kAnalogThreshold));
        aj.up = (dat->joy_rel_y < (kAnalogCenter - kAnalogThreshold));

        aj.fire = dat->button_y || dat->button_a;
        aj.sec_fire = dat->button_b;
        aj.auto_fire = dat->button_x;

        aj.joystick_swap = dat->button_select;

        if (target_) {
            target_->process_gamepad_report(aj);
        }
    }

    ReportType expected_report() override {
        return kGamePad;
    }
};

static HidHandlerBuilder builder(0x0079, 0x0011, []() { return std::make_unique<HizueHidHandler>(); }, nullptr);
