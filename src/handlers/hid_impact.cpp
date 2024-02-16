/**
 * @file hid_impact.cpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "hid_handler.hpp"

#include "bsp/board.h"
#include "controller_port.hpp"
#include "pico/stdlib.h"

struct __attribute__((packed)) Report {
    uint8_t joy_left_x;
    uint8_t joy_left_y;
    uint8_t joy_right_y;
    uint8_t joy_right_x;

    // Byte 4
    uint8_t coolie_hat : 4;
    uint8_t button_left : 1;
    uint8_t button_top : 1;
    uint8_t button_bottom : 1;
    uint8_t button_right : 1;

    // Byte 5
    uint8_t trigger_l1 : 1;
    uint8_t trigger_l2 : 1;
    uint8_t trigger_r1 : 1;
    uint8_t trigger_r2 : 1;
    uint8_t button_select : 1;
    uint8_t button_start : 1;
    uint8_t stick_click_left : 1;
    uint8_t stick_click_right : 1;
};

/**
 * @brief Driver for the noname impact Controller
 * Is a very early cheap USB clone of the Playstation 1 controller.
 */
class ImpactHidHandler : public DefaultHidHandler {
    void process_report(std::span<const uint8_t> d) {

        auto dat = reinterpret_cast<const Report *>(d.data());

#if 0
        PRINTF("Impact: %d %d%d%d%d", dat->coolie_hat, dat->button_left,
               dat->button_right, dat->button_top, dat->button_bottom);
        for (uint8_t i : d) {
            PRINTF(" %02x", i);
        }
        PRINTF("\n");
#endif

        GamepadReport aj;

        aj.update_from_coolie_hat(dat->coolie_hat); // Coolie Hat D-Pad

        aj.fire = dat->button_left || dat->button_right;
        aj.sec_fire = dat->button_bottom;
        aj.auto_fire = dat->button_top;

        aj.joystick_swap = dat->button_select;

        if (target_) {
            target_->process_gamepad_report(aj);
        }
    }

    ReportType expected_report() override {
        return kGamePad;
    }
};

static HidHandlerBuilder builder(
    0x07b5, 0x0314, []() { return std::make_unique<ImpactHidHandler>(); }, nullptr);