/**
 * @file hid_ps3.cpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "default_hid_handler.hpp"
#include "hid_handler_builder.hpp"

#include "pico/stdlib.h"
#include "tusb.h"

#include "controller_port.hpp"

/**
 * @brief Packed struct representing PS3 DualShock HID report
 */
struct __attribute__((packed)) Report {
    uint8_t reserved1[2];

    // Byte 2
    uint8_t button_select : 1;
    uint8_t stick_click_left : 1;
    uint8_t stick_click_right : 1;
    uint8_t button_start : 1;
    uint8_t dpad_up : 1;
    uint8_t dpad_right : 1;
    uint8_t dpad_down : 1;
    uint8_t dpad_left : 1;

    // Byte 3
    uint8_t trigger_l2 : 1;
    uint8_t trigger_r2 : 1;
    uint8_t trigger_l1 : 1;
    uint8_t trigger_r1 : 1;

    uint8_t button_triangle : 1;
    uint8_t button_circle : 1;
    uint8_t button_cross : 1;
    uint8_t button_square : 1;

    // Bytes 4,5
    uint8_t reserved3[2];

    // Bytes 6,7,8,9
    uint8_t joy_left_x;
    uint8_t joy_left_y;
    uint8_t joy_right_x;
    uint8_t joy_right_y;
};

/**
 * @brief Driver for the PS3 Dual Shock Controller
 *
 */
class PS3DualShockHandler : public DefaultHidHandler {

  private:
    /// @brief Neutral position of a joystick axis.
    static constexpr uint32_t kAnalogCenter{0x80};

    /// @brief Minimum difference to center to register a direction
    static constexpr uint32_t kAnalogThreshold{0x40};

    /**
     * @brief Enables PS3 Controller
     *
     * Got this from
     * https://github.com/felis/USB_Host_Shield_2.0/blob/master/PS3USB.cpp#L491
     * @param dev_addr
     * @param instance
     */
    void enable_sixaxis(int8_t dev_addr, uint8_t instance) {
        static uint8_t cmd_buf[4];
        cmd_buf[0] = 0x42; // Special PS3 Controller enable commands
        cmd_buf[1] = 0x0c;
        cmd_buf[2] = 0x00;
        cmd_buf[3] = 0x00;

        if (!tuh_hid_set_report(dev_addr, instance, 0xf4, 0x03, cmd_buf, 4)) {
            PRINTF("Error: cannot send tuh_hid_set_report\n");
        }
    }

  public:
    void process_report(std::span<const uint8_t> d) override {

        auto dat = reinterpret_cast<const Report *>(d.data());

#if 0
        PRINTF("PS3: %d%d%d%d %d%d%d%d %d%d%d%d ", dat->dpad_up,
               dat->dpad_right, dat->dpad_down, dat->dpad_left,
               dat->button_triangle, dat->button_circle, dat->button_cross,
               dat->button_square, dat->trigger_l1, dat->trigger_l2,
               dat->trigger_r1, dat->trigger_r2);
        for (uint8_t i : d) {
            PRINTF(" %02x", i);
        }
        PRINTF("\n");
#endif

        GamepadReport aj;

        // The Dual shock doesn't use a coolie hat for the D-Pad
        // instead it is handled like 4 buttons
        aj.left = dat->dpad_left || (dat->joy_left_x < (kAnalogCenter - kAnalogThreshold));
        aj.down = dat->dpad_down || (dat->joy_left_y > (kAnalogCenter + kAnalogThreshold));
        aj.right = dat->dpad_right || (dat->joy_left_x > (kAnalogCenter + kAnalogThreshold));
        aj.up = dat->dpad_up || (dat->joy_left_y < (kAnalogCenter - kAnalogThreshold));

        aj.fire = dat->button_square || dat->button_circle;
        aj.sec_fire = dat->button_cross;
        aj.auto_fire = dat->button_triangle;

        aj.joystick_swap = dat->button_select;

        if (target_) {
            target_->process_gamepad_report(aj);
        }
    }

    void setup_reception(int8_t dev_addr, uint8_t instance) override {
        enable_sixaxis(dev_addr, instance);

        if (!tuh_hid_receive_report(dev_addr, instance)) {
            PRINTF("Error: cannot request to receive report\n");
        }
    }

    ReportType expected_report() override {
        return kGamePad;
    }
};

static HidHandlerBuilder builder(
    0x054c, 0x0268, []() { return std::make_unique<PS3DualShockHandler>(); }, nullptr);
