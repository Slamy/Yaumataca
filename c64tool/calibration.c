#include <6502.h>
#include <c64.h>
#include <conio.h>
#include <mouse.h>
#include <stdint.h>
#include <stdio.h>

uint8_t pot_x;
uint8_t pot_y;
uint8_t port;

enum CalibrationState {
    kIdle,             ///< Just doing the job it is supposed to do
    kCalibratePotX64,  ///< Calibrate lower end of usable Pot X range
    kCalibratePotX191, ///< Calibrate upper end of usable Pot X range
    kCalibratePotY64,  ///< Calibrate lower end of usable Pot Y range
    kCalibratePotY191, ///< Calibrate upper end of usable Pot Y range
};

enum CalibrationState calibration_state;
enum CalibrationState displayed_calibration_state;

uint8_t pot_in_calibration_range() {
    return (pot_x > 20 && pot_x < 210 && pot_y > 20 && pot_y < 210);
}

uint8_t pot_in_c1351_range() {
    return (pot_x > 40 && pot_x < 210 && pot_y > 40 && pot_y < 210);
}

void calculate_calibration_state() {
    if (pot_x < 30 && pot_y < 128) {
        calibration_state = kCalibratePotY64;
    } else if (pot_x < 30 && pot_y >= 128) {
        calibration_state = kCalibratePotY191;
    } else if (pot_y < 30 && pot_x < 128) {
        calibration_state = kCalibratePotX64;
    } else if (pot_y < 30 && pot_x >= 128) {
        calibration_state = kCalibratePotX191;
    } else {
        calibration_state = kIdle;
    }
}

void display_calibration_state() {
    gotoxy(0, 10);

    switch (calibration_state) {
    case kIdle:
        cputs("Waiting for calibration mode...        \r\n"
              "To activate it press                   \r\n"
              "left left right right right wheel      \r\n"
              "on your mouse.\r\n");
        break;
    case kCalibratePotX64:
        cputs("Use wheel to make Pot X stable 64.     \r\n"
              "Press left mouse button to continue    \r\n"
              "Press right mouse button to abort      \r\n");
        cclearxy(0, 13, 40);

        break;
    case kCalibratePotX191:
        cputs("Use wheel to make Pot X stable 191.   \r\n");
        break;
    case kCalibratePotY64:
        cputs("Use wheel to make Pot Y stable 64.    \r\n");
        break;
    case kCalibratePotY191:
        cputs("Use wheel to make Pot Y stable 191.   \r\n");
        break;
    }
}

static uint8_t display_min_pot_x;
static uint8_t display_max_pot_x;
static uint8_t display_min_pot_y;
static uint8_t display_max_pot_y;
static uint8_t display_samples;

static uint8_t work_min_pot_x;
static uint8_t work_max_pot_x;
static uint8_t work_min_pot_y;
static uint8_t work_max_pot_y;
static uint8_t work_samples;

static uint8_t last_time;

void measure_sample() {
    pot_x = SID.ad1; // read pot x
    pot_y = SID.ad2; // read pot y
    work_samples++;

    if (CIA1.tod_sec != last_time) {
        last_time = CIA1.tod_sec;
        display_min_pot_x = work_min_pot_x;
        display_max_pot_x = work_max_pot_x;
        display_min_pot_y = work_min_pot_y;
        display_max_pot_y = work_max_pot_y;
        display_samples = work_samples;

        work_min_pot_x = pot_x;
        work_max_pot_x = pot_x;
        work_min_pot_y = pot_y;
        work_max_pot_y = pot_y;
        work_samples = 0;
    }

    if (pot_x > work_max_pot_x)
        work_max_pot_x = pot_x;
    if (pot_x < work_min_pot_x)
        work_min_pot_x = pot_x;

    if (pot_y > work_max_pot_y)
        work_max_pot_y = pot_y;
    if (pot_y < work_min_pot_y)
        work_min_pot_y = pot_y;
}

void measure_mouse() {
    if (port == 1)
        CIA1.pra = 0b01111111; // Select Controller Port 1
    else
        CIA1.pra = 0b10111111; // Select Controller Port 2

    // make sure that we have proper measure cycles
    waitvsync();
    waitvsync();

    pot_x = SID.ad1; // read pot x
    pot_y = SID.ad2; // read pot y

    // force refresh of the description
    displayed_calibration_state = 0xff;

    // reset statistics
    work_min_pot_x = pot_x;
    work_max_pot_x = pot_x;
    work_min_pot_y = pot_y;
    work_max_pot_y = pot_y;
    display_min_pot_x = 0;
    display_max_pot_x = 0;
    display_min_pot_y = 0;
    display_max_pot_y = 0;

    CIA1.tod_10 = 0; // start the time of day counter

    while (pot_in_calibration_range()) {
        textcolor(1);
        gotoxy(0, 6);

        // for benchmarking
        // cprintf("          Min Cur Max %3d\r\n", display_samples);
        cputs("         Min Cur Max    \r\n");

        measure_sample();

        if (calibration_state == kCalibratePotY191 || calibration_state == kCalibratePotY64)
            textcolor(11);
        else
            textcolor(1);
        cprintf(" Pot X   %3d %3d %3d\r\n", display_min_pot_x, pot_x, display_max_pot_x);

        measure_sample();

        if (calibration_state == kCalibratePotX191 || calibration_state == kCalibratePotX64)
            textcolor(11);
        else
            textcolor(1);

        cprintf(" Pot Y   %3d %3d %3d\r\n", display_min_pot_y, pot_y, display_max_pot_y);
        measure_sample();

        textcolor(1);

        calculate_calibration_state();
        if (calibration_state != displayed_calibration_state) {
            displayed_calibration_state = calibration_state;
            display_calibration_state();
        }
    }

    // clear description
    cclearxy(0, 10, 40);
    cclearxy(0, 11, 40);
    cclearxy(0, 12, 40);
    cclearxy(0, 13, 40);
}

uint8_t wait_for_mouse() {
    static uint8_t confidence1;
    static uint8_t confidence2;

    gotoxy(0, 4);
    cprintf("Looking for a mouse...             ");
    cclearxy(0, 8, 23);

    confidence1 = 0;
    confidence2 = 0;

    while (confidence1 < 5 && confidence2 < 5) {
        CIA1.pra = 0b01111111; // Select Controller Port 1
        waitvsync();

        pot_x = SID.ad1; // read pot x
        pot_y = SID.ad2; // read pot y

        gotoxy(0, 6);
        cprintf(" Port 1  %3d %3d            \r\n", pot_x, pot_y);

        if (pot_in_c1351_range())
            confidence1++;
        else if (confidence1 > 0)
            confidence1--;

        CIA1.pra = 0b10111111; // Select Controller Port 2
        waitvsync();

        pot_x = SID.ad1; // read pot x
        pot_y = SID.ad2; // read pot y

        gotoxy(0, 7);
        cprintf(" Port 2  %3d %3d            \r\n", pot_x, pot_y);

        if (pot_in_c1351_range())
            confidence2++;
        else if (confidence2 > 0)
            confidence2--;
    }

    port = confidence1 >= confidence2 ? 1 : 2;
    gotoxy(0, 4);
    cprintf("Mouse plugged into port %d detected!\r\n", port);
    return port;
}

void main(void) {
    clrscr();        // clear screen
    bordercolor(12); // gray border
    bgcolor(0);      // black background
    textcolor(1);    // white text

    printf("Welcome to the Yaumataca calibration\n"
           "tool! Let's calibrate the C1351 mode.\n");

    SEI();            // disable IRQs to avoid enabling keyboard scan
    CIA1.ddra = 0xc0; // Disable keyboard scan

    for (;;) {
        wait_for_mouse();
        measure_mouse();
    }
}