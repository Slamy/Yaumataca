
#pragma once

#include "mouse_quadrature.hpp"
#include "processors/interfaces.hpp"
#include "utility.h"

class AmigaMouse : public QuadratureMouse {
  private:
    ControllerPortState wheel_state_;
    ControllerPortState last_wheel_state_;

  public:
    AmigaMouse() { PRINTF("AmigaMouse +\n"); }
    virtual ~AmigaMouse() { PRINTF("AmigaMouse -\n"); }

    /**
     * @brief Update period
     * Tested in Amiga workbench with permament maximum speed over seconds.
     * Period of 359 us is too fast. 532 us too.
     * 110 will lead to a period of 704 us, which is ok. No glitches.
     */
    static constexpr uint32_t kUpdatePeriod = 110;

    void run() override {
        uint32_t now = board_micros();
        uint32_t time_diff = now - last_update;

        if (time_diff >= kUpdatePeriod) {
            last_update = now;
            auto h_state = h.update();
            auto v_state = v.update();
            auto wheel_state = wheel.update();

            state_.up = v_state.second;
            state_.down = h_state.second;
            state_.left = v_state.first;
            state_.right = h_state.first;

            wheel_state_.up = wheel_state.second;
            wheel_state_.left = wheel_state.first;

            if (mouse_target_ && last_state_ != state_) {

                last_state_ = state_;
                mouse_target_->set_port_state(state_);
            }
            if (wheel_target_ && last_wheel_state_ != wheel_state_) {
                last_wheel_state_ = wheel_state_;

                wheel_target_->set_port_state(wheel_state_);
            }
        }
    }
};
