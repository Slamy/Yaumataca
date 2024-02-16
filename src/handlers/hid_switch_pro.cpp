/**
 * @file hid_switch_pro.cpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-18
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "hid_handler.hpp"

#include "bsp/board.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "controller_port.hpp"

// much code and constants from
// https://github.com/Dan611/hid-procon/blob/master/hid-procon.c
// https://github.com/felis/USB_Host_Shield_2.0/blob/master/SwitchProParser.h

/// Report ID for outgoing reports
#define PROCON_REPORT_SEND_USB 0x80

/// ID of reports containing the button presses we are looking for
#define PROCON_REPORT_INPUT_FULL 0x30

/// First command to activate USB mode
#define PROCON_USB_HANDSHAKE 0x02

/// Second command to activate USB mode
#define PROCON_USB_ENABLE 0x04

/// TODO unknown
#define PROCON_USB_DO_CMD 0x92

/// TODO unknown
#define PROCON_CMD_AND_RUMBLE 0x01

/// Command to set the front LEDs
#define PROCON_CMD_LED 0x30

/**
 * @brief Packed struct representing Switch Pro Controller Buttons
 * reduced from
 * https://github.com/felis/USB_Host_Shield_2.0/blob/master/SwitchProParser.h
 */
struct __attribute__((packed)) SwitchProButtons {
    uint8_t y : 1; ///< Y Button
    uint8_t x : 1; ///< X Button
    uint8_t b : 1; ///< B Button
    uint8_t a : 1; ///< A Button

    uint8_t dummy1 : 2; ///< reserved
    uint8_t r : 1;      ///< unused
    uint8_t zr : 1;     ///< unused

    uint8_t minus : 1; ///< Minus / Select Button
    uint8_t plus : 1;  ///< unused
    uint8_t r3 : 1;    ///< unused
    uint8_t l3 : 1;    ///< unused

    uint8_t home : 1;    ///< unused
    uint8_t capture : 1; ///< unused
    uint8_t dummy2 : 2;  ///< reserved

    uint8_t dpad_down : 1;  ///< D Pad Down
    uint8_t dpad_up : 1;    ///< D Pad Up
    uint8_t dpad_right : 1; ///< D Pad Right
    uint8_t dpad_left : 1;  ///< D Pad Left

    uint8_t dummy3 : 2; ///< reserved
    uint8_t l : 1;      ///< unused
    uint8_t zl : 1;     ///< unused
};

/**
 * @brief Packed struct representing the standard Switch Pro Controller HID
 * Report
 *
 * reduced from
 * https://github.com/felis/USB_Host_Shield_2.0/blob/master/SwitchProParser.h
 */
struct __attribute__((packed)) SwitchProData {
    uint8_t input_report_id;     ///< has to be 0x30 for a valid report
    uint8_t timer;               ///< incrementing counter
    uint8_t connection_info : 4; ///< unused
    uint8_t battery_level : 4;   ///< unused

    /// Button and joystick values
    SwitchProButtons btn; // Bytes 3-5

    // Bytes 6-11
    uint16_t leftHatX : 12;  ///< Left Joystick X
    uint16_t leftHatY : 12;  ///< Left Joystick Y
    uint16_t rightHatX : 12; ///< Right Joystick X
    uint16_t rightHatY : 12; ///< Right Joystick Y
};

/**
 * @brief Driver for the Nintendo Switch Pro Controller
 *
 */
class SwitchProHandler : public DefaultHidHandler {
  private:
    /// @brief Milliseconds between commands sent
    /// The Pro Controller seems to ignore commands without any pauses
    /// Empirically tested
    static constexpr uint32_t kPauseBetweenCommands{80};

    /// @brief true if USB Handshake is performed
    bool handshake_sent_{false};

    /// @brief true if USB timeouts were disabled
    bool timeout_disabled_{false};

    /// @brief true if first LED is set
    bool led_set_{false};

    int8_t dev_addr_; ///< TinyUSB internal
    int8_t instance_; ///< TinyUSB internal

    /// Number of reports we have received until now
    uint32_t reports_collected_{0};

    /// Absolute time in milliseconds when the most recent command was
    /// transmitted to the controller
    uint32_t last_command_sent_{0};

    /// @brief Neutral position of a joystick axis.
    static constexpr uint32_t kAnalogCenter{2048};

    /// @brief Minimum difference to center to register a direction
    static constexpr uint32_t kAnalogThreshold{500};

    /**
     * @brief Activates first LED
     *
     * inspired by
     * https://github.com/Dan611/hid-procon/blob/master/hid-procon.c#L166
     */
    void set_first_led(int8_t dev_addr, uint8_t instance) {
        static uint8_t buf[30];
        buf[0] = PROCON_REPORT_SEND_USB;
        buf[1] = PROCON_USB_DO_CMD; // PROCON_USB_DO_CMD
        buf[2] = 0x00;
        buf[3] = 0x31; // undocumented magic number
        buf[4] = 0x00;
        buf[5] = 0x00;
        buf[6] = 0x00;
        buf[7] = 0x00;

        buf[8 + 0x00] = PROCON_CMD_AND_RUMBLE; // Report ID
        buf[8 + 0x01] = 0;
        // Left rumble data
        buf[8 + 0x02 + 0] = 0x00;
        buf[8 + 0x02 + 1] = 0x10; // undocumented
        buf[8 + 0x02 + 2] = 0x40; // undocumented
        buf[8 + 0x02 + 3] = 0x40; // undocumented
        // Right rumble data
        buf[8 + 0x02 + 4] = 0x00;
        buf[8 + 0x02 + 5] = 0x10; // undocumented
        buf[8 + 0x02 + 6] = 0x40; // undocumented
        buf[8 + 0x02 + 7] = 0x40; // undocumented

        buf[8 + 0x0A + 0] = PROCON_CMD_LED;
        buf[8 + 0x0A + 1] = 1; // only must left LED

        if (!tuh_hid_send_report(dev_addr, instance, 0, buf, 8 + 12)) {
            PRINTF("Error: cannot set LED\n");
        } else {
            led_set_ = true;
            PRINTF("LEDs set\n");
        }
    }
    /**
     * @brief
     *
     * Got this from
     * https://github.com/felis/USB_Host_Shield_2.0/blob/master/SwitchProUSB.h
     *
     * @param dev_addr
     * @param instance
     */
    void send_handshake(int8_t dev_addr, uint8_t instance) {
        static uint8_t buf[2];
        buf[0] = PROCON_REPORT_SEND_USB;
        buf[1] = PROCON_USB_HANDSHAKE;

        if (!tuh_hid_send_report(dev_addr, instance, 0, buf, 2)) {
            PRINTF("Error: cannot send handshake\n");
        } else {
            PRINTF("Handshake sent\n");
            handshake_sent_ = true;
        }
    }

    /**
     * @brief
     *
     * Got this from
     * https://github.com/felis/USB_Host_Shield_2.0/blob/master/SwitchProUSB.h
     *
     * @param dev_addr
     * @param instance
     */
    void disable_timeout(int8_t dev_addr, uint8_t instance) {
        static uint8_t buf[2];
        buf[0] = PROCON_REPORT_SEND_USB;
        buf[1] = PROCON_USB_ENABLE;

        if (!tuh_hid_send_report(dev_addr, instance, 0, buf, 2)) {
            PRINTF("Error: cannot disable timeout\n");
        } else {
            timeout_disabled_ = true;
            PRINTF("Timeouts disabled\n");
        }
    }

    void run() override {
        uint32_t now = board_millis();

        if (now - last_command_sent_ > kPauseBetweenCommands) {
            last_command_sent_ = now;
            if (!handshake_sent_) {
                send_handshake(dev_addr_, instance_);
            } else if (!timeout_disabled_) {
                disable_timeout(dev_addr_, instance_);
            }
        }
    };

  public:
    SwitchProHandler() {
        last_command_sent_ = board_millis();
    }
    void process_report(std::span<const uint8_t> d) override {

        auto dat = reinterpret_cast<const SwitchProData *>(d.data());

#if 0

        PRINTF("Switch: %d %d %d %d %d%d%d%d\n", dat->leftHatX, dat->leftHatY,
               dat->rightHatX, dat->rightHatY, dat->btn.dpad_down,
               dat->btn.dpad_up, dat->btn.dpad_left, dat->btn.dpad_right);
        for (uint8_t i : d) {
            PRINTF(" %02x", i);
        }
        PRINTF("\n");
#endif

        GamepadReport aj;

        if (dat->input_report_id == PROCON_REPORT_INPUT_FULL) {
            aj.left = dat->btn.dpad_left || dat->leftHatX < (kAnalogCenter - kAnalogThreshold);
            aj.right = dat->btn.dpad_right || dat->leftHatX > (kAnalogCenter + kAnalogThreshold);
            aj.up = dat->btn.dpad_up || dat->leftHatY > (kAnalogCenter + kAnalogThreshold);
            aj.down = dat->btn.dpad_down || dat->leftHatY < (kAnalogCenter - kAnalogThreshold);

            aj.fire = dat->btn.y;
            aj.sec_fire = dat->btn.x;
            aj.auto_fire = dat->btn.b;

            aj.joystick_swap = dat->btn.minus;

            if (target_) {
                target_->process_gamepad_report(aj);
            }
        }
        reports_collected_++;

        // activating the LEDs too early seems to shut down the controller
        if (!led_set_ && reports_collected_ > 20) {
            set_first_led(dev_addr_, instance_);
        }
    }

    void setup_reception(int8_t dev_addr, uint8_t instance) override {
        if (!tuh_hid_receive_report(dev_addr, instance)) {
            PRINTF("Error: cannot request to receive report\n");
        }
        reports_collected_ = 0;
        dev_addr_ = dev_addr;
        instance_ = instance;
    }

    ReportType expected_report() override {
        return kGamePad;
    }
};

static HidHandlerBuilder builder(
    0x057e, 0x2009, []() { return std::make_unique<SwitchProHandler>(); }, nullptr);
