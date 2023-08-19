/**
 * @file joystick_mouse_switcher.hpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "interfaces.hpp"

/**
 * @brief Detects mouse and joystick handling and changes the data source.
 * Supports one mouse source and one joystick source with one destination.
 */
class JoystickMouseSwitcher : public ReportHubInterface {
  private:
    /// @brief Currently activated device type. Mouse or Joystick
    ReportType active_ = {kGamePad};

    /**
     * @brief Minimum relative movement to switch to mouse
     * Analyzed on report basis. A high DPI mouse will probably reach this
     * faster. Value was empirically constructed by "feel"
     */
    static constexpr uint32_t kMouseChangeThreshold = 6;

    /// @brief Used to keep track if my mouse source exists
    std::weak_ptr<HidHandlerInterface> mouse_source_;

    /// @brief Used to keep track if my joystick source exists
    std::weak_ptr<HidHandlerInterface> joystick_source_;

  public:
    JoystickMouseSwitcher() { PRINTF("JoystickMouseSwitcher +\n"); }
    virtual ~JoystickMouseSwitcher() { PRINTF("JoystickMouseSwitcher -\n"); }

    /// @brief Sink for mouse reports
    std::shared_ptr<RunnableMouseReportProcessor> mouse_target_;

    /// @brief Sink for gamepad reports
    std::shared_ptr<RunnableGamepadReportProcessor> gamepad_target_;

    /// @brief Returns true if no mouse source is registered
    bool mouse_source_empty() { return mouse_source_.expired(); }

    /// @brief Returns true if no joystick source is registered
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
            PRINTF("Switched to gamepad\n");
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
            PRINTF("Switched to mouse\n");
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
        } else if (gamepad_target_ && active_ == kGamePad) {
            gamepad_target_->run();
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

    /**
     * @brief Ensures correct output pin configuration
     *
     * for currently selected data source
     */
    void ensure_muxing() {
        ensure_mouse_muxing();
        ensure_joystick_muxing();
    }
};
