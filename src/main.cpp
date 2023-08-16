/**
 * @file main.cpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp/board.h"
#include "controller_port.hpp"
#include "hid_app.hpp"
#include "pico/stdlib.h"
#include "processors/joystick_mouse_switcher.hpp"
#include "processors/mouse_c1351.hpp"
#include "tusb.h"
#include "utility.h"
//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

PIO C1351Converter::pio_{nullptr};
uint C1351Converter::offset_{0};

/*------------- MAIN -------------*/

/**
 * @brief First function to call
 *
 * Will never return.
 * @return int
 */
int main() {
    board_init();

    printf("Yaumataca says hello!\n");

    // init host stack on configured roothub port
    tuh_init(BOARD_TUH_RHPORT);

    C1351Converter::setup_pio();

    hid_app_init();

    for (;;) {
        // tinyusb host task
        tuh_task();
#if CFG_TUH_HID
        hid_app_task();
#endif
    }
}
