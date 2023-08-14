#pragma once

#include "interfaces.hpp"

class JoystickMouseSwitcher : public ReportHubInterface {
  private:
    ReportType active_ = {kGamePad};

    static constexpr uint32_t kMouseChangeThreshold = 6;

    std::weak_ptr<HidHandlerInterface> mouse_source_;
    std::weak_ptr<HidHandlerInterface> joystick_source_;

  public:
    JoystickMouseSwitcher() { printf("JoystickMouseSwitcher +\n"); }
    virtual ~JoystickMouseSwitcher() { printf("JoystickMouseSwitcher -\n"); }

    std::shared_ptr<MouseReportProcessor> mouse_target_;
    std::shared_ptr<GamepadReportProcessor> gamepad_target_;

    bool mouse_source_empty() { return mouse_source_.expired(); }

    bool joystick_source_empty() { return joystick_source_.expired(); }

    void register_source(std::shared_ptr<HidHandlerInterface> source) override {
        switch (source->expected_report()) {
        case kMouse:
            mouse_source_ = source;
            break;
        case kGamePad:
            joystick_source_ = source;
            break;
        }
    }

    void process_gamepad_report(GamepadReport &report) override {

        if (report.button_pressed && active_ != kGamePad) {
            printf("Switched to gamepad\n");
            active_ = kGamePad;
            gamepad_target_->ensure_joystick_muxing();
        }
        if (gamepad_target_ && active_ == kGamePad) {
            gamepad_target_->process_gamepad_report(report);
        }
    }

    void process_mouse_report(MouseReport &report) override {

        if (((labs(report.relx) > kMouseChangeThreshold) ||
             (labs(report.rely) > kMouseChangeThreshold) ||
             report.button_pressed) &&
            active_ != kMouse) {
            printf("Switched to mouse\n");
            active_ = kMouse;
            mouse_target_->ensure_mouse_muxing();
        }
        if (mouse_target_ && active_ == kMouse) {
            mouse_target_->process_mouse_report(report);
        }
    }

    void run() override {
        if (mouse_target_ && active_ == kMouse) {
            mouse_target_->run();
        }
    }

    void ensure_mouse_muxing() override {
        if (active_ == kMouse)
            mouse_target_->ensure_mouse_muxing();
    }

    void ensure_joystick_muxing() override {
        if (active_ == kGamePad)
            gamepad_target_->ensure_joystick_muxing();
    }

    void ensure_muxing() {
        ensure_mouse_muxing();
        ensure_joystick_muxing();
    }
};
