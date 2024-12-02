/**
 * @file mouse_quadrature.hpp
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
#include "quadrature_encoder.hpp"

/**
 * @brief Shared code between \ref AmigaMouse and \ref AtariStMouse
 * Provides 3 quadrature encoders and mouse button handling which is equal for
 * both systems.
 */
class QuadratureMouse : public RunnableMouseReportProcessor {
  protected:
    QuadratureEncoder h;     ///< horizontal movement
    QuadratureEncoder v;     ///< vertical movement
    QuadratureEncoder wheel; ///< wheel movement (only used by \ref AmigaMouse)
    /// absolute time in microseconds when the output has changed
    uint32_t last_update{0};

    ControllerPortState state_;      ///< current state of the controller port
    ControllerPortState last_state_; ///< last state to check for changes

  public:
    QuadratureMouse() {
        PRINTF("QuadratureMouse +\n");
    }
    virtual ~QuadratureMouse() {
        PRINTF("QuadratureMouse -\n");
    }

    /// @brief Destination of mouse buttons and quadrature signals
    std::shared_ptr<ControllerPortInterface> mouse_target_;
#if CONFIG_DISABLE_AMIGA_WHEELBUSMOUSE == 0
    /// @brief Destination of wheel quadrature signals
    std::shared_ptr<ControllerPortInterface> wheel_target_;
#endif

    void process_mouse_report(MouseReport &mouse_report) override {
        h.add_to_accumulator(mouse_report.relx);
        v.add_to_accumulator(mouse_report.rely);
        wheel.add_to_accumulator(mouse_report.wheel);

        state_.fire1 = mouse_report.left;
        state_.fire2 = mouse_report.right;
        state_.fire3 = mouse_report.middle;

        if (mouse_target_ && last_state_ != state_) {
            last_state_ = state_;
            mouse_target_->set_port_state(state_);
        }
    }

    void ensure_mouse_muxing() override {
        if (mouse_target_)
            mouse_target_->configure_gpios();
#if CONFIG_DISABLE_AMIGA_WHEELBUSMOUSE == 0
        if (wheel_target_)
            wheel_target_->configure_gpios();
#endif
    }
};
