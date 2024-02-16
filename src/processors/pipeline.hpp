/**
 * @file pipeline.hpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "utility.h"

#include "gamepad_autofire.hpp"
#include "interfaces.hpp"
#include "joystick_mouse_switcher.hpp"
#include "led_task.hpp"
#include "mouse_amiga.hpp"
#include "mouse_atarist.hpp"
#include "mouse_c1351.hpp"
#include "mouse_mode_switcher.hpp"
#include "port_switcher.hpp"
#include "small_fee.hpp"

/**
 * @brief Central manager of data flow in this project
 * Connects the various components in meaningful manner.
 */
class Pipeline : public Runnable {
  private:
    /// @brief Controller Port 1, Mouse Port, Right Port
    std::shared_ptr<JoystickMouseSwitcher> primary_mouse_switcher_{std::make_shared<JoystickMouseSwitcher>()};

    /// @brief Controller Port 2, Joystick Port, Left Port
    std::shared_ptr<JoystickMouseSwitcher> primary_joystick_switcher_{std::make_shared<JoystickMouseSwitcher>()};

    /// @brief Proxy object which relays incoming controller port data
    std::shared_ptr<PortSwitcher> joystick_port_;
    /// @brief Proxy object which relays incoming controller port data
    std::shared_ptr<PortSwitcher> mouse_port_;

    /// @brief Proxy object which selects a mouse driver
    std::shared_ptr<MouseModeSwitcher> mouse_switcher1_;
    /// @brief Proxy object which selects a mouse driver
    std::shared_ptr<MouseModeSwitcher> mouse_switcher2_;

    /// @brief Auto fire implementation
    std::shared_ptr<GamepadAutoFire> autofire1;
    /// @brief Auto fire implementation
    std::shared_ptr<GamepadAutoFire> autofire2;

    /// @brief List of all objects that require constant attention
    std::vector<std::shared_ptr<Runnable>> runnables_;

    /// @brief Makes the LED of the Pico blink
    LedPatternGenerator led_pattern_;
    /// @brief Writes configuration data
    SingleByteFlashEepromEmulation fee_;

    /// @brief Currently selected mouse type. Ranges 0-2
    int mouse_mode_{0};

    /// @brief Is true, if configuration has to be written back
    bool mouse_mode_dirty_{false};

    /// @brief Absolute time in milliseconds when to write the configuration
    uint32_t mouse_mode_write_back_at_{0};

  public:
    virtual ~Pipeline() {
        PRINTF("Pipeline -\n");
    }

    /**
     * @brief Performs controller port swapping
     * Also ensures that GPIOs are correctly mixed
     */
    void swap_callback() {
        led_pattern_.set_pattern(LedPatternGenerator::k2Long);

        if (mouse_port_)
            mouse_port_->swap();

        if (primary_joystick_switcher_)
            primary_joystick_switcher_->ensure_muxing();

        if (primary_mouse_switcher_)
            primary_mouse_switcher_->ensure_muxing();
    }

    /**
     * @brief Construct a new Pipeline object
     *
     * @param joystick_port     Primary jostick port
     * @param mouse_port        Primary mouse port
     */
    Pipeline(std::shared_ptr<ControllerPortInterface> joystick_port,
             std::shared_ptr<ControllerPortInterface> mouse_port) {
        PRINTF("Pipeline +\n");

        mouse_mode_ = fee_.get_config_byte();

        joystick_port->configure_gpios();
        mouse_port->configure_gpios();

        auto port_switcher = PortSwitcher::construct_pair(mouse_port, joystick_port);
        mouse_port_ = port_switcher.first;
        joystick_port_ = port_switcher.second;

        mouse_switcher1_ = std::make_shared<MouseModeSwitcher>();
        mouse_switcher2_ = std::make_shared<MouseModeSwitcher>();
        autofire1 = std::make_shared<GamepadAutoFire>();
        autofire2 = std::make_shared<GamepadAutoFire>();

        autofire1->set_swap_callback(std::bind(&Pipeline::swap_callback, this));
        autofire2->set_swap_callback(std::bind(&Pipeline::swap_callback, this));
        mouse_switcher1_->set_swap_callback(std::bind(&Pipeline::swap_callback, this));
        mouse_switcher2_->set_swap_callback(std::bind(&Pipeline::swap_callback, this));

        primary_mouse_switcher_->mouse_target_ = mouse_switcher1_;
        primary_mouse_switcher_->gamepad_target_ = autofire2;

        primary_joystick_switcher_->mouse_target_ = mouse_switcher2_;
        primary_joystick_switcher_->gamepad_target_ = autofire1;

        mouse_switcher1_->mouse_target_ = mouse_port_;
        mouse_switcher1_->wheel_target_ = joystick_port_;
        mouse_switcher2_->mouse_target_ = joystick_port_;

        autofire2->target_ = mouse_port_;
        autofire1->target_ = joystick_port_;

        mouse_switcher1_->set_mode(mouse_mode_);
        mouse_switcher2_->set_mode(mouse_mode_);

        runnables_.push_back(primary_mouse_switcher_);
        runnables_.push_back(primary_joystick_switcher_);
    }

    /**
     * @brief Switches to next mouse mode
     *
     * Cycles though Amiga, Atari ST and C1351.
     * Ensures the correct muxing of the output pins
     * Starts a 10 second timer upon expiration the config is stored in flash
     */
    void cycle_mouse_mode() {
        PRINTF("Cycle mouse mode!\n");
        mouse_mode_ = (mouse_mode_ + 1) % MouseModeSwitcher::number_modes();

        mouse_mode_write_back_at_ = board_millis() + 1000 * 10;
        mouse_mode_dirty_ = true;

        mouse_switcher1_->set_mode(mouse_mode_);
        mouse_switcher2_->set_mode(mouse_mode_);

        primary_joystick_switcher_->ensure_muxing();
        primary_mouse_switcher_->ensure_muxing();

        switch (mouse_mode_) {
        case 0:
            led_pattern_.set_pattern(LedPatternGenerator::k3Short); // Amiga
            break;
        case 1:
            led_pattern_.set_pattern(LedPatternGenerator::kMorseR); // Atari ST
            break;
        case 2:
            led_pattern_.set_pattern(LedPatternGenerator::k2Long); // C1351
            break;
        }
    }

    /**
     * @brief Integrates a new HID handler into the pipeline
     *
     * Only 2 mouses and 2 joysticks can be integrated.
     * More will be ignored.
     * @param handler handler to integrate
     */
    void integrate_handler(std::shared_ptr<HidHandlerInterface> handler) {

        switch (handler->expected_report()) {
        case kMouse:

            if (primary_mouse_switcher_->mouse_source_empty()) {
                PRINTF("Primary mouse\n");

                handler->set_target(primary_mouse_switcher_);
                primary_mouse_switcher_->register_source(handler);
            } else if (primary_joystick_switcher_->mouse_source_empty()) {
                PRINTF("Secondary mouse\n");

                handler->set_target(primary_joystick_switcher_);
                primary_joystick_switcher_->register_source(handler);
            } else {
                PRINTF("Mouse ignored!\n");
            }

            break;
        case kGamePad:
            if (primary_joystick_switcher_->joystick_source_empty()) {
                PRINTF("Primary joystick\n");

                handler->set_target(primary_joystick_switcher_);
                primary_joystick_switcher_->register_source(handler);
            } else if (primary_mouse_switcher_->joystick_source_empty()) {
                PRINTF("Secondary joystick\n");

                handler->set_target(primary_mouse_switcher_);
                primary_mouse_switcher_->register_source(handler);
            } else {
                PRINTF("Joystick ignored!\n");
            }

            break;
        }

        led_pattern_.set_pattern(LedPatternGenerator::k1Short);
    }

    void run() override {
        led_pattern_.run();
        for (auto &i : runnables_) {
            i->run();
        }

        if (mouse_mode_dirty_ && board_millis() > mouse_mode_write_back_at_) {
            PRINTF("Write mouse_mode to flash!\n");

            fee_.write_config(static_cast<uint8_t>(mouse_mode_));
            mouse_mode_dirty_ = false;
        }
    }
};
