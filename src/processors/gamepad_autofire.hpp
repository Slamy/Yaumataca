
#pragma once

#include "bsp/board.h"
#include "processors/interfaces.hpp"
#include <functional>

class GamepadAutoFire : public GamepadReportProcessor, public Runnable {
  private:
    uint32_t last_update{0};

    GamepadReport in_state_;
    ControllerPortState out_state_;
    ControllerPortState last_out_state_;
    bool auto_fire_state_{false};
    uint32_t auto_fire_period_{100};
    uint32_t joystick_swap_cnt_{0};

    static constexpr uint32_t kJoystickSwapThreshold{7};
    std::function<void()> swap_callback_;

  public:
    GamepadAutoFire() { PRINTF("GamepadAutoFire +\n"); }
    virtual ~GamepadAutoFire() { PRINTF("GamepadAutoFire -\n"); }

    void set_swap_callback(std::function<void()> swap_callback) {
        swap_callback_ = swap_callback;
    }

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
