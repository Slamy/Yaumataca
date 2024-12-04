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
    ReportType active_;

    /**
     * @brief Minimum relative movement to switch to mouse
     * Analyzed on report basis. A high DPI mouse will probably reach this
     * faster. Value was empirically constructed by "feel"
     */
    static constexpr uint32_t kMouseChangeThreshold = 6;

    /// @brief Used to keep track if my mouse source exists
    std::weak_ptr<ReportSourceInterface> mouse_source_;

    /// @brief Used to keep track if my joystick source exists
    std::weak_ptr<ReportSourceInterface> joystick_source_;

  public:
    /**
     * @brief Construct a new Joystick Mouse Switcher
     *
     * @param initial_report_type   Report type to start with
     */
    JoystickMouseSwitcher(ReportType initial_report_type) : active_(initial_report_type) {
        PRINTF("JoystickMouseSwitcher +\n");
    }
    virtual ~JoystickMouseSwitcher() {
        PRINTF("JoystickMouseSwitcher -\n");
    }

    /// @brief Sink for mouse reports
    std::shared_ptr<RunnableMouseReportProcessor> mouse_target_;

    /// @brief Sink for gamepad reports
    std::shared_ptr<RunnableGamepadReportProcessor> gamepad_target_;

    /// @brief Sink for gamepad reports to the sibling
    /// Used to activate the FC3 hack on the other port
    std::shared_ptr<GamePadFeatures> other_gamepad_target_;

    /// @brief Returns true if no mouse source is registered
    bool mouse_source_empty() {
        return mouse_source_.expired();
    }

    /// @brief Returns true if no joystick source is registered
    bool joystick_source_empty() {
        return joystick_source_.expired();
    }

    /**
     * @brief Removes mouse source from this switcher
     *
     * @return Mouse source. Can be empty pointer.
     */
    std::shared_ptr<ReportSourceInterface> take_mouse_source() {
        std::shared_ptr<ReportSourceInterface> temp = mouse_source_.lock();
        mouse_source_.reset();
        return temp;
    }

    /**
     * @brief Removes joystick source from this switcher
     *
     * @return Joystick source. Can be empty pointer
     */
    std::shared_ptr<ReportSourceInterface> take_joystick_source() {
        std::shared_ptr<ReportSourceInterface> temp = joystick_source_.lock();
        joystick_source_.reset();
        return temp;
    }

    void register_source(std::shared_ptr<ReportSourceInterface> source) override {
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
        // If any button or D-Pad direction is pressend,
        // do the switch
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

        if (((labs(report.relx) > kMouseChangeThreshold) || (labs(report.rely) > kMouseChangeThreshold) ||
             report.button_pressed) &&
            active_ != kMouse) {
            PRINTF("Switched to mouse\n");
            active_ = kMouse;
            mouse_target_->ensure_mouse_muxing();
        }
        if (mouse_target_ && active_ == kMouse) {
            mouse_target_->process_mouse_report(report);
            other_gamepad_target_->final_cart_hack();
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
        if (mouse_target_ && active_ == kMouse)
            mouse_target_->ensure_mouse_muxing();
    }

    void ensure_joystick_muxing() override {
        if (gamepad_target_ && active_ == kGamePad)
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
