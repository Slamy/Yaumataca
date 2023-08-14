

#pragma once

#include "interfaces.hpp"
#include "mouse_amiga.hpp"
#include "mouse_atarist.hpp"
#include "mouse_c1351.hpp"

class MouseModeSwitcher : public MouseReportProcessor {

  private:
    std::shared_ptr<MouseReportProcessor> impl_;

    bool swap_combination_pressed_{false};
    bool swap_performed_{false};
    uint32_t swap_press_start_time{0};

    std::function<void()> swap_callback_;
    static constexpr uint32_t kJoystickSwapThreshold{200};

  public:
    void set_swap_callback(std::function<void()> swap_callback) {
        swap_callback_ = swap_callback;
    }

    MouseModeSwitcher() { printf("MouseModeSwitcher +\n"); }
    virtual ~MouseModeSwitcher() { printf("MouseModeSwitcher -\n"); }

    std::shared_ptr<ControllerPortInterface> mouse_target_;
    std::shared_ptr<ControllerPortInterface> wheel_target_;

    static int number_modes() { return 3; }

    void set_mode(int mode) {
        switch (mode) {
        case 0: {
            auto impl = std::make_shared<AmigaMouse>();
            impl->mouse_target_ = mouse_target_;
            impl->wheel_target_ = wheel_target_;
            impl_ = impl;
            break;
        }
        case 1: {
            auto impl = std::make_shared<AtariStMouse>();
            impl->mouse_target_ = mouse_target_;
            impl_ = impl;
            break;
        }
        case 2: {
            auto impl = std::make_shared<C1351Converter>();
            impl->set_target(mouse_target_);
            impl_ = impl;
            break;
        }
        }
    }

    void ensure_mouse_muxing() override { impl_->ensure_mouse_muxing(); }

    void process_mouse_report(MouseReport &mouse_report) override {

        bool swap_combi =
            (mouse_report.left && mouse_report.middle && mouse_report.right);

        if (!swap_combination_pressed_ && swap_combi) {
            swap_press_start_time = board_millis();
            swap_performed_ = false;
        }
        swap_combination_pressed_ = swap_combi;

        if (impl_)
            impl_->process_mouse_report(mouse_report);
    }

    void run() override {

        if (swap_combination_pressed_ && !swap_performed_) {
            uint32_t now = board_millis();

            if ((now - swap_press_start_time) > 300) {
                printf("Mouse Swap!\n");
                swap_performed_ = true;
                if (swap_callback_) {
                    swap_callback_();
                }
            }
        }

        if (impl_)
            impl_->run();
    }
};
