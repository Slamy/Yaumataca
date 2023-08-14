/// @file pipeline.hpp

#pragma once

#include "gamepad_autofire.hpp"
#include "interfaces.hpp"
#include "joystick_mouse_switcher.hpp"
#include "led_task.hpp"
#include "mouse_amiga.hpp"
#include "mouse_atarist.hpp"
#include "mouse_c1351.hpp"
#include "mouse_mode_switcher.hpp"
#include "port_switcher.hpp"

class Pipeline : public Runnable {
  private:
    std::shared_ptr<JoystickMouseSwitcher> primary_mouse_switcher_{
        std::make_shared<JoystickMouseSwitcher>()};

    std::shared_ptr<JoystickMouseSwitcher> primary_joystick_switcher_{
        std::make_shared<JoystickMouseSwitcher>()};

    std::shared_ptr<PortSwitcher> joystick_port_;
    std::shared_ptr<PortSwitcher> mouse_port_;

    std::shared_ptr<MouseModeSwitcher> mouse_switcher1_;
    std::shared_ptr<MouseModeSwitcher> mouse_switcher2_;

    std::shared_ptr<GamepadAutoFire> autofire1;
    std::shared_ptr<GamepadAutoFire> autofire2;

    std::vector<std::shared_ptr<Runnable>> runnables_;

    LedPatternGenerator led_pattern_;
    int mouse_mode_{2};

  public:
    Pipeline() { printf("Pipeline +\n"); }
    virtual ~Pipeline() { printf("Pipeline -\n"); }

    void swap_callback() {
        led_pattern_.set_pattern(LedPatternGenerator::k2Long);

        if (mouse_port_)
            mouse_port_->swap();

        if (primary_joystick_switcher_)
            primary_joystick_switcher_->ensure_muxing();

        if (primary_mouse_switcher_)
            primary_mouse_switcher_->ensure_muxing();
    }

    Pipeline(std::shared_ptr<ControllerPortInterface> joystick_port,
             std::shared_ptr<ControllerPortInterface> mouse_port) {
        joystick_port->configure_gpios();
        mouse_port->configure_gpios();

        auto port_switcher =
            PortSwitcher::construct_pair(mouse_port, joystick_port);
        mouse_port_ = port_switcher.first;
        joystick_port_ = port_switcher.second;

        mouse_switcher1_ = std::make_shared<MouseModeSwitcher>();
        mouse_switcher2_ = std::make_shared<MouseModeSwitcher>();
        autofire1 = std::make_shared<GamepadAutoFire>();
        autofire2 = std::make_shared<GamepadAutoFire>();

        autofire1->set_swap_callback(std::bind(&Pipeline::swap_callback, this));
        autofire2->set_swap_callback(std::bind(&Pipeline::swap_callback, this));
        mouse_switcher1_->set_swap_callback(
            std::bind(&Pipeline::swap_callback, this));
        mouse_switcher2_->set_swap_callback(
            std::bind(&Pipeline::swap_callback, this));

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

        runnables_.push_back(mouse_switcher1_);
        runnables_.push_back(mouse_switcher2_);
        runnables_.push_back(autofire1);
        runnables_.push_back(autofire2);
    }

    void cycle_mouse_mode() {
        printf("Cycle mouse mode!\n");
        mouse_mode_ = (mouse_mode_ + 1) % MouseModeSwitcher::number_modes();
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

    void integrate_handler(std::shared_ptr<HidHandlerInterface> handler) {

        switch (handler->expected_report()) {
        case kMouse:

            if (primary_mouse_switcher_->mouse_source_empty()) {
                printf("Primary mouse\n");

                handler->set_target(primary_mouse_switcher_);
                primary_mouse_switcher_->register_source(handler);
            } else if (primary_joystick_switcher_->mouse_source_empty()) {
                printf("Secondary mouse\n");

                handler->set_target(primary_joystick_switcher_);
                primary_joystick_switcher_->register_source(handler);
            } else {
                printf("Mouse ignored!\n");
            }

            break;
        case kGamePad:
            if (primary_joystick_switcher_->joystick_source_empty()) {
                printf("Primary joystick\n");

                handler->set_target(primary_joystick_switcher_);
                primary_joystick_switcher_->register_source(handler);
            } else if (primary_mouse_switcher_->joystick_source_empty()) {
                printf("Secondary joystick\n");

                handler->set_target(primary_mouse_switcher_);
                primary_mouse_switcher_->register_source(handler);
            } else {
                printf("Joystick ignored!\n");
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
    }
};
