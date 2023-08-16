/**
 * @file gamepad_autofire.hpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include "bsp/board.h"
#include "processors/interfaces.hpp"
#include <functional>

/**
 * @brief Derives additional actions from Joystick input
 * Implements auto fire and detects intent to swap controller ports.
 */
class GamepadAutoFire : public GamepadReportProcessor, public Runnable {
  private:
    /// @brief absolute time in milliseconds when auto fire was handled last
    uint32_t last_update{0};

    /// @brief latest presented state of the gamepad button state
    GamepadReport in_state_;

    /// @brief current output to controller port
    ControllerPortState out_state_;
    /// @brief last output to controller port. used to check for changes
    ControllerPortState last_out_state_;

    /// @brief current state of autofire output
    bool auto_fire_state_{false};

    /// @brief Half period of auto fire in milliseconds
    uint32_t auto_fire_period_{100};

    /// @brief Increment every \ref auto_fire_period_ when select is held
    /// Is checked against \ref kJoystickSwapThreshold
    uint32_t joystick_swap_cnt_{0};

    /// @brief Waiting time until swapping is performed
    /// Duration is measured in units of \ref auto_fire_period_
    static constexpr uint32_t kJoystickSwapThreshold{7};

    /// @brief Called when select button sis held for some time
    std::function<void()> swap_callback_;

  public:
    GamepadAutoFire() { PRINTF("GamepadAutoFire +\n"); }
    virtual ~GamepadAutoFire() { PRINTF("GamepadAutoFire -\n"); }

    /**
     * @brief Registers callback handler for port swap intent
     *
     * Callback is called when the Select button is hold for some time
     *
     * @param swap_callback function pointer
     */
    void set_swap_callback(std::function<void()> swap_callback) {
        swap_callback_ = swap_callback;
    }

    /// @brief controller port to feed with generated button states
    std::shared_ptr<ControllerPortInterface> target_;

    void process_gamepad_report(GamepadReport &report) override {
        in_state_ = report;
    }

    void run() override {
        uint32_t now = board_millis();

        uint32_t time_diff = now - last_update;

        if (time_diff > auto_fire_period_) {
            last_update = now;
            auto_fire_state_ = !auto_fire_state_;

            if (in_state_.joystick_swap) {
                joystick_swap_cnt_++;
                if (joystick_swap_cnt_ == kJoystickSwapThreshold) {
                    PRINTF("Joystick Swap!\n");

                    if (swap_callback_) {
                        swap_callback_();
                    }
                }
            } else {
                joystick_swap_cnt_ = 0;
            }
        }

        if (in_state_.auto_fire) {
            out_state_.fire1 = auto_fire_state_;
        } else {
            out_state_.fire1 = in_state_.fire;
        }

        out_state_.fire2 = in_state_.sec_fire;
        out_state_.up = in_state_.up;
        out_state_.down = in_state_.down;
        out_state_.left = in_state_.left;
        out_state_.right = in_state_.right;

        if (target_ && last_out_state_ != out_state_) {
            last_out_state_ = out_state_;
            target_->set_port_state(out_state_);
        }
    }

    void ensure_joystick_muxing() override { target_->configure_gpios(); }
};
