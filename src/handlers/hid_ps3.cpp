#include "hid.hpp"

#include "bsp/board.h"
#include "pico/stdlib.h"
#include "tusb.h"

class PS3DualShockHandler : public HidHandler {
    void process_report(std::span<const uint8_t> d) override {
#if 0
        printf("PS3:");
        for (uint8_t i : d) {
            printf(" %02x", i);
        }
        printf("\r\n");
#endif
        AmigaJoyPort aj;

        // The Dual shock doesn't use coolie hat for the D-Pad
        // instead they are handled like normal buttons
        aj.left = d[2] & 0x80;
        aj.down = d[2] & 0x40;
        aj.right = d[2] & 0x20;
        aj.up = d[2] & 0x10;

        aj.fire = d[3] & 0x80;    // Square
        aj.secFire = d[3] & 0x10; // Triangle

        gpio_put(0, aj.right);
        gpio_put(1, aj.left);
        gpio_put(2, aj.down);
        gpio_put(3, aj.up);
        gpio_put(4, aj.fire);
        gpio_put(6, aj.secFire);
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
            printf("Error: cannot send tuh_hid_set_report\r\n");
        }
    }
};

static HidHandlerBuilder builder(
    0x054c, 0x0268, []() { return std::make_unique<PS3DualShockHandler>(); },
    nullptr);