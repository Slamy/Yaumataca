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
#include "tusb.h"

/**
 * @brief Driver for the noname impact Controller
 * Is a very early cheap USB clone of the Playstation 1 controller.
 */
class ImpactHidHandler : public DefaultHidHandler {
    void process_report(std::span<const uint8_t> d) {

#ifdef DEBUG_PRINT
        PRINTF("Impact:");
        for (uint8_t i : d) {
            PRINTF(" %02x", i);
        }
        PRINTF("\n");
#endif

        GamepadReport aj;

        uint8_t hat_dir = (d[4] & 0x0F); // Coolie Hat D-Pad
        aj.update_from_coolie_hat(hat_dir);

        aj.fire = d[4] & 0x10;
        aj.sec_fire = d[4] & 0x20;
        aj.auto_fire = d[4] & 0x40;

        aj.joystick_swap = d[5] & 0x10; // Select button

        if (target_) {
            target_->process_gamepad_report(aj);
        }
    }

    ReportType expected_report() override { return kGamePad; }
};

static HidHandlerBuilder builder(
    0x07b5, 0x0314, []() { return std::make_unique<ImpactHidHandler>(); },
    nullptr);