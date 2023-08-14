
#pragma once

#include "processors/interfaces.hpp"
#include "quadrature_encoder.hpp"

class QuadratureMouse : public MouseReportProcessor {
  protected:
    QuadratureEncoder h;
    QuadratureEncoder v;
    QuadratureEncoder wheel;
    uint32_t last_update{0};

    ControllerPortState state_;
    ControllerPortState last_state_;

  public:
    QuadratureMouse() { printf("QuadratureMouse +\n"); }
    virtual ~QuadratureMouse() { printf("QuadratureMouse -\n"); }

    std::shared_ptr<ControllerPortInterface> mouse_target_;
    std::shared_ptr<ControllerPortInterface> wheel_target_;

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

        if (wheel_target_)
            wheel_target_->configure_gpios();
    }
};
