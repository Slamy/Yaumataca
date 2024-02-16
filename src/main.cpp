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

#include "controller_port.hpp"
#include "hid_api.hpp"
#include "pico/stdlib.h"
#include "processors/mouse_c1351.hpp"
#include "processors/pipeline.hpp"
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

    for (;;) {
        // tinyusb host task
        tuh_task();
        hid_app_task();
        Pipeline::getInstance().run();

        static uint32_t button_debounce_cnt = 0;
        static uint32_t last_button_state = 0;
        bool button_state = board_button_read();

        if (!last_button_state && button_state && button_debounce_cnt == 0) {
            Pipeline::getInstance().cycle_mouse_mode();
            button_debounce_cnt = 100;
        } else if (button_debounce_cnt > 0 && !button_state) {
            button_debounce_cnt--;
        }
        last_button_state = button_state;
    }
}
