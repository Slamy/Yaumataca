/**
 * @file gamepad_features.hpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include "processors/interfaces.hpp"
#include "utility.h"
#include <functional>

/**
 * @brief Derives additional actions from Joystick input
 * Implements auto fire and detects intent to swap controller ports.
 */
class GamePadFeatures : public RunnableGamepadReportProcessor {
  private:
    /// @brief absolute time in milliseconds when auto fire was handled last
    uint32_t last_update{0};

    /// @brief latest presented state of the gamepad button state
    GamepadReport in_state_;

    /// @brief current output to controller port
    ControllerPortState out_state_;
    /// @brief last output to controller port. used to check for changes
    ControllerPortState last_out_state_;

    /**
     * @brief Workaround for mouse issues with Final Cart III
     *
     * The FC3 module desktop mode muxes both controller ports
     * to the POT X and Y lines of the SID.
     * It is incompatible with two mice and the way, the Yaumataca simulates
     * a C1351. This hack enforces to "let go" of these lines which
     * indirectly might cause issues with C64 games that support
     * additional fire buttons as those are always depressed now.
     *
     * Only effective in C64 mode
     */
    bool final_cart_hack_active_{false};

    /// @brief current state of autofire output
    bool auto_fire_state_{false};

    /// @brief Half period of auto fire in milliseconds
    uint32_t auto_fire_period_{30};

    /// @brief Increment every \ref auto_fire_period_ when select is held
    /// Is checked against \ref kJoystickSwapThreshold
    uint32_t joystick_swap_cnt_{0};

    /// @brief Waiting time until swapping is performed
    /// Duration is measured in units of \ref auto_fire_period_
    static constexpr uint32_t kJoystickSwapThreshold{7};

    /// @brief Called when select button sis held for some time
    std::function<void()> swap_callback_;

    /// @brief True for C64 mode. Refer to \ref set_c64_mode
    bool c64_mode_{false};

  public:
    /**
     * @brief C64 type handling
     *
     * The 2. and 3. fire buttons are different
     * for the C64 compared to Amiga and Atari ST.
     * The pressed state is high instead of low.
     * Activating the C64 mode abstracts
     * this behaviour away in this class.
     *
     * @param m True for C64 mode
     */
    void set_c64_mode(bool m) {
        c64_mode_ = m;
        // enforce writing the state
        last_out_state_.all_buttons = 0xff;
    }

    GamePadFeatures() {
        PRINTF("GamePadFeatures +\n");
    }
    virtual ~GamePadFeatures() {
        PRINTF("GamePadFeatures -\n");
    }

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
        if (final_cart_hack_active_ && (report.sec_fire || report.third_fire)) {
            final_cart_hack_active_ = false;
            PRINTF("Deactivate FC3 Hack!\n");
        }

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

        if (c64_mode_) {
            if (final_cart_hack_active_) {
                // Let go of the lines to avoid draining them.
                // Fixes problem with SID POT muxing and FC3
                out_state_.fire2 = 0;
                out_state_.fire3 = 0;
            } else {
                out_state_.fire2 = !in_state_.sec_fire;
                out_state_.fire3 = !in_state_.third_fire;
            }
        } else {
            out_state_.fire2 = in_state_.sec_fire;
            out_state_.fire3 = in_state_.third_fire;
        }
        out_state_.up = in_state_.up;
        out_state_.down = in_state_.down;
        out_state_.left = in_state_.left;
        out_state_.right = in_state_.right;

        if (target_ && last_out_state_ != out_state_) {
            last_out_state_ = out_state_;
            target_->set_port_state(out_state_);
        }
    }

    /// @brief Activates Final Cart III hack
    /// Will be deactivated if 2. or 3. button is pressed
    void final_cart_hack() {
        if (!final_cart_hack_active_) {
            PRINTF("Activate FC3 Hack!\n");
        }

        final_cart_hack_active_ = true;
    }

    void ensure_joystick_muxing() override {
        target_->configure_gpios();
        // enforce writing the state
        last_out_state_.all_buttons = 0xff;
    }
};
