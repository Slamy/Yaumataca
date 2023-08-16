/**
 * @file mouse_mode_switcher.hpp
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
#include "mouse_amiga.hpp"
#include "mouse_atarist.hpp"
#include "mouse_c1351.hpp"

/**
 * @brief Proxy class of \ref MouseReportProcessor for multiple implementations
 * Can act as multiple different types of mouses.
 *
 * Will call a swap callback handler if all 3 buttons are pressed for some time.
 */
class MouseModeSwitcher : public MouseReportProcessor {

  private:
    /// @brief contains the currently used mouse implementation
    std::shared_ptr<MouseReportProcessor> impl_;

    /// @brief true if 3 buttons of the mouse are pressed
    bool swap_combination_pressed_{false};

    /// @brief Used to keep track if the "3 button swap" is performed.
    bool swap_performed_{false};

    /// @brief absolute time in milliseconds when all 3 buttons were pressed
    uint32_t swap_press_start_time{0};

    /// @brief function which will be called to implement the "3 button swap"
    std::function<void()> swap_callback_;

    /// @brief "3 button" minimum holding time in milliseconds
    static constexpr uint32_t kJoystickSwapThreshold{200};

  public:
    /**
     * @brief Sets callback to swap controller ports
     *
     * Called when all 3 mouse buttons are pressed to indicate the intent
     * to swap controller ports.
     *
     * @param swap_callback function pointer or lambda
     */
    void set_swap_callback(std::function<void()> swap_callback) {
        swap_callback_ = swap_callback;
    }

    MouseModeSwitcher() { PRINTF("MouseModeSwitcher +\n"); }
    virtual ~MouseModeSwitcher() { PRINTF("MouseModeSwitcher -\n"); }

    /// @brief Destination of mouse buttons and quadrature signals
    std::shared_ptr<ControllerPortInterface> mouse_target_;

    /// @brief Destination of wheel quadrature signals
    std::shared_ptr<ControllerPortInterface> wheel_target_;

    /// @brief Provides number of mouse types supported
    /// @return number of mouse types
    static int number_modes() { return 3; }

    /**
     * @brief Sets a type of mouse
     *
     * @param mode      a value in the range of 0-2
     */
    void set_mode(int mode) {
        PRINTF("Set mouse mode %d\n", mode);

        switch (mode) {
        case 0: {
            auto impl = std::make_shared<AmigaMouse>();
            impl->mouse_target_ = mouse_target_;
            impl->wheel_target_ = wheel_target_;
            impl_ = impl;
            break;
        }
        case 1: {
            auto impl = std::make_shared<AtariStMouse>();
            impl->mouse_target_ = mouse_target_;
            impl_ = impl;
            break;
        }
        case 2:
        default: {
            auto impl = std::make_shared<C1351Converter>();
            impl->set_target(mouse_target_);
            impl_ = impl;
            break;
        }
        }
    }

    void ensure_mouse_muxing() override { impl_->ensure_mouse_muxing(); }

    void process_mouse_report(MouseReport &mouse_report) override {

        bool swap_combi =
            (mouse_report.left && mouse_report.middle && mouse_report.right);

        if (!swap_combination_pressed_ && swap_combi) {
            swap_press_start_time = board_millis();
            swap_performed_ = false;
        }
        swap_combination_pressed_ = swap_combi;

        if (impl_)
            impl_->process_mouse_report(mouse_report);
    }

    void run() override {

        if (swap_combination_pressed_ && !swap_performed_) {
            uint32_t now = board_millis();

            if ((now - swap_press_start_time) > 300) {
                PRINTF("Mouse Swap!\n");
                swap_performed_ = true;
                if (swap_callback_) {
                    swap_callback_();
                }
            }
        }

        if (impl_)
            impl_->run();
    }
};
