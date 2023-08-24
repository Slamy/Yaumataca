/**
 * @file mouse_atarist.hpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include "mouse_quadrature.hpp"
#include "processors/interfaces.hpp"

/**
 * @brief Emulation of an Atari ST Mouse
 *
 * Provides quadrature encoded signals. Very similar to the \ref AmigaMouse
 * but with a different pinout. (Why did they do this?)
 * Were they afraid that people would connect an Amiga mouse to an Atari ST and
 * vice versa?
 */
class AtariStMouse : public QuadratureMouse {
  public:
    AtariStMouse() { PRINTF("AtariStMouse +\n"); }
    virtual ~AtariStMouse() { PRINTF("AtariStMouse -\n"); }

    /**
     * @brief Update period
     * Tested with real Atari 1040STFM with GEM on Low Resolution.
     * 350 is ok. 310 is not ok. 330 is also not ok. 340 is not ok, leading
     * to 1.44ms period. We are going for 360 here which is a period of 1.7ms
     * Or maybe even 420 wich also leads to 1.7ms but more stable as it seems.
     * 425 leads to 1.78ms which seems to be more stable than 1.7ms in the long
     * run.
     * 450 leads to 2ms which is even more stable. Hallucination?
     */
    static constexpr uint32_t kUpdatePeriod = 450;

    void run() override {
        uint32_t now = board_micros();
        uint32_t time_diff = now - last_update;

        if (time_diff > kUpdatePeriod) {
            last_update = now;
            auto h_state = h.update();
            auto v_state = v.update();

            state_.up = h_state.first;
            state_.down = h_state.second;
            state_.left = v_state.first;
            state_.right = v_state.second;

            if (mouse_target_ && last_state_ != state_) {

                last_state_ = state_;
                mouse_target_->set_port_state(state_);
            }
        }
    }
};
