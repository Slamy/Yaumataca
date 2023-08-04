#include "hid.hpp"

#include "bsp/board.h"
#include "pico/stdlib.h"
#include "tusb.h"

class ImpactHidHandler : public HidHandler {
    void process_report(std::span<const uint8_t> d) {

        printf("Impact:");
        for (uint8_t i : d) {
            printf(" %02x", i);
        }
        printf("\r\n");

        AmigaJoyPort aj;

        int hat_dir = (d[4] & 0x0F); // Coolie Hat D-Pad
        aj.up = (hat_dir == 8 || hat_dir == 1 || hat_dir == 2);
        aj.right = (hat_dir == 2 || hat_dir == 3 || hat_dir == 4);
        aj.down = (hat_dir == 4 || hat_dir == 5 || hat_dir == 6);
        aj.left = (hat_dir == 6 || hat_dir == 7 || hat_dir == 8);

        aj.fire = d[4] & 0x10;    // 1
        aj.secFire = d[4] & 0x20; // 7

        gpio_put(0, aj.right);
        gpio_put(1, aj.left);
        gpio_put(2, aj.down);
        gpio_put(3, aj.up);
        gpio_put(4, aj.fire);
        gpio_put(6, aj.secFire);

        /*
        gpio_put(15, aj.right);
        gpio_put(14, aj.left);
        gpio_put(12, aj.down);
        gpio_put(10, aj.up);
        gpio_put(9, aj.fire);
        gpio_put(7, aj.secFire);
        */
    }
};

static HidHandlerBuilder builder(
    0x07b5, 0x0314, []() { return std::make_unique<ImpactHidHandler>(); },
    nullptr);