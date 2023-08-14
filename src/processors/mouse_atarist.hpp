

#pragma once

#include "mouse_quadrature.hpp"
#include "processors/interfaces.hpp"

class AtariStMouse : public QuadratureMouse {
  public:
    AtariStMouse() { printf("AtariStMouse +\n"); }
    virtual ~AtariStMouse() { printf("AtariStMouse -\n"); }

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
