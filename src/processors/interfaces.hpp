
#pragma once

#include <span>

enum ReportType { kMouse, kGamePad };

class MouseReport {
  public:
    union {
        struct {
            uint8_t left : 1;
            uint8_t right : 1;
            uint8_t middle : 1;
            uint8_t reserved : 5;
        };
        uint8_t button_pressed{0};
    };
    int8_t relx{0}, rely{0}, wheel{0};
};

class GamepadReport {
  public:
    union {
        struct {
            bool fire : 1;
            bool sec_fire : 1;
            bool auto_fire : 1;
            bool up : 1;
            bool down : 1;
            bool left : 1;
            bool right : 1;
            bool joystick_swap : 1;
        };
        uint32_t button_pressed{0};
    };

    void update_from_coolie_hat(uint8_t hat_dir) {
        up = (hat_dir == 8 || hat_dir == 1 || hat_dir == 2);
        right = (hat_dir == 2 || hat_dir == 3 || hat_dir == 4);
        down = (hat_dir == 4 || hat_dir == 5 || hat_dir == 6);
        left = (hat_dir == 6 || hat_dir == 7 || hat_dir == 8);
    }
};

struct ControllerPortState {
    union {
        struct {
            unsigned int up : 1;
            unsigned int down : 1;
            unsigned int left : 1;
            unsigned int right : 1;
            unsigned int fire1 : 1;
            unsigned int fire2 : 1;
            unsigned int fire3 : 1;
        };
        uint8_t all_buttons{0};
    };
};

inline bool operator==(const ControllerPortState &lhs,
                       const ControllerPortState &rhs) {
    return lhs.all_buttons == rhs.all_buttons;
}

class Runnable {
  public:
    virtual void run() = 0;
};

class ReportHubInterface;

class HidHandlerInterface {
  public:
    virtual void setup_reception(int8_t dev_addr, uint8_t instance) = 0;
    virtual void process_report(std::span<const uint8_t> report) = 0;
    virtual ReportType expected_report() = 0;
    virtual void set_target(std::shared_ptr<ReportHubInterface> target) = 0;
};

class GamepadReportProcessor {
  public:
    virtual void process_gamepad_report(GamepadReport &mouse_report) = 0;
    virtual void ensure_joystick_muxing() = 0;
};

class MouseReportProcessor : public Runnable {
  public:
    virtual void process_mouse_report(MouseReport &mouse_report) = 0;
    virtual void ensure_mouse_muxing() = 0;
};

class ReportHubInterface : public MouseReportProcessor,
                           public GamepadReportProcessor {

  public:
    virtual void
    register_source(std::shared_ptr<HidHandlerInterface> source) = 0;
};

/**
 * Represents something which offers access to the outer world, driving an atari
 * type joystick port. Also has some helper functions for implementing C1351
 * signals.
 */

class ControllerPortInterface {
  public:
    virtual void set_port_state(ControllerPortState &state) = 0;
    virtual uint get_pot_x_drain_gpio() = 0;
    virtual uint get_pot_y_drain_gpio() = 0;
    virtual uint get_pot_y_sense_gpio() = 0;
    virtual void configure_gpios() = 0;
    virtual const char *get_name() = 0;
};