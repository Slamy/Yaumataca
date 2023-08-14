#include "hid_handler.hpp"

#include "bsp/board.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "controller_port.hpp"

class PS3DualShockHandler : public DefaultHidHandler {
    void process_report(std::span<const uint8_t> d) override {

#if 0
        printf("PS3:");
        for (uint8_t i : d) {
            printf(" %02x", i);
        }
        printf("\n");
#endif
        GamepadReport aj;

        // The Dual shock doesn't use a coolie hat for the D-Pad
        // instead it is handled like 4 buttons
        aj.left = d[2] & 0x80;
        aj.down = d[2] & 0x40;
        aj.right = d[2] & 0x20;
        aj.up = d[2] & 0x10;

        aj.fire = d[3] & 0x80;     // Square
        aj.sec_fire = d[3] & 0x10; // Triangle

        aj.joystick_swap = d[2] & 0x01; // Select

        if (target_) {
            target_->process_gamepad_report(aj);
        }
    }

    void setup_reception(int8_t dev_addr, uint8_t instance) override {
        // Got this from
        // https://github.com/felis/USB_Host_Shield_2.0/blob/master/PS3USB.cpp#L491
        static uint8_t cmd_buf[4];
        cmd_buf[0] = 0x42; // Special PS3 Controller enable commands
        cmd_buf[1] = 0x0c;
        cmd_buf[2] = 0x00;
        cmd_buf[3] = 0x00;

        if (!tuh_hid_set_report(dev_addr, instance, 0xf4, 0x03, cmd_buf, 4)) {
            printf("Error: cannot send tuh_hid_set_report\n");
        }
    }

    ReportType expected_report() override { return kGamePad; }
};

static HidHandlerBuilder builder(
    0x054c, 0x0268, []() { return std::make_unique<PS3DualShockHandler>(); },
    nullptr);