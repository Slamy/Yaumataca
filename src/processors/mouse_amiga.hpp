/**
 * @file mouse_amiga.hpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include "config.h"
#include "mouse_quadrature.hpp"
#include "processors/interfaces.hpp"
#include "utility.h"

/**
 * @brief Emulation of an Amiga Mouse
 *
 * Provides quadrature encoded signals on one port while also producing
 * wheel quadrature signals for usage with this driver:
 * http://aminet.net/package/util/mouse/WheelBusMouse
 *
 */
class AmigaMouse : public QuadratureMouse {
  private:
    /// current state of the second controller port
    ControllerPortState wheel_state_;

    /// last state to check for changes
    ControllerPortState last_wheel_state_;

  public:
    AmigaMouse() {
        PRINTF("AmigaMouse +\n");
    }
    virtual ~AmigaMouse() {
        PRINTF("AmigaMouse -\n");
    }

    /**
     * @brief Update period
     * Tested in Amiga workbench on A1200
     * with permament maximum speed over seconds.
     * Period of 359 us is too fast. 532 us too.
     * 170 will lead to a period of 704 us, which is ok. No glitches.
     */
    static constexpr uint32_t kUpdatePeriod = 170;

    void run() override {
        uint32_t now = board_micros();
        uint32_t time_diff = now - last_update;

        if (time_diff >= kUpdatePeriod) {
            last_update = now;
            auto h_state = h.update();
            auto v_state = v.update();
            auto wheel_state = wheel.update();

            // Vertical movement
            state_.up = v_state.second;
            state_.left = v_state.first;

            // Horizontal movement
            state_.right = h_state.first;
            state_.down = h_state.second;

            // Wheel movement (non standard)
            wheel_state_.up = wheel_state.second;
            wheel_state_.left = wheel_state.first;

            if (mouse_target_ && last_state_ != state_) {
                last_state_ = state_;
                mouse_target_->set_port_state(state_);
            }

#if CONFIG_FORCE_MOUSE_BOOT_MODE == 0
            if (wheel_target_ && last_wheel_state_ != wheel_state_) {
                last_wheel_state_ = wheel_state_;
                wheel_target_->set_port_state(wheel_state_);
            }
#endif
        }
    }
};
