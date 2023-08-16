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
     * TODO Test with real Atari ST
     */
    static constexpr uint32_t kUpdatePeriod = 110;

    void run() override {
        uint32_t now = board_micros();
        uint32_t time_diff = now - last_update;

        if (time_diff > 20) {
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
