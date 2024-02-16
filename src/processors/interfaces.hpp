/**
 * @file interfaces.hpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <cstdint>
#include <memory>
#include <span>

/// Types of reports which are supported
enum ReportType { kMouse, kGamePad };

/**
 * @brief Reduction of a Mouse HID Report to the required essentials.
 */
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
    int8_t relx{0};  ///< relative x movement
    int8_t rely{0};  ///< relative y movement
    int8_t wheel{0}; ///< relative wheel movement
};

/**
 * @brief Reduction of a Joystick HID Report to the required essentials.
 * Does contain all the button presses, that are required by other algorithms
 * down the line.
 */
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

    /**
     * @brief Helper function to fill directional data via "coolie hat" value
     *
     * Coolie hat data is represented as 4 bit field in the report,
     * with a value in the range of 0 if not depressed or 1-8 if pressed.
     *
     * @param hat_dir   raw coolie hat value from report data
     */
    void update_from_coolie_hat(uint8_t hat_dir) {
        up = (hat_dir == 8 || hat_dir == 1 || hat_dir == 2);
        right = (hat_dir == 2 || hat_dir == 3 || hat_dir == 4);
        down = (hat_dir == 4 || hat_dir == 5 || hat_dir == 6);
        left = (hat_dir == 6 || hat_dir == 7 || hat_dir == 8);
    }
};

/**
 * @brief Representation of a signal state of an Atari type controller port
 * A 1 state in this struct represents a pressed button and thus a low state on
 * the physical level.
 */
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

/**
 * @brief Operator overloading for ==
 *
 * @param lhs   a controller state
 * @param rhs   another controller state to compare to
 * @return true   If both are equal
 * @return false  If both are not equal
 */
inline bool operator==(const ControllerPortState &lhs, const ControllerPortState &rhs) {
    return lhs.all_buttons == rhs.all_buttons;
}

/**
 * @brief Interface for classes that require frequent attention
 */
class Runnable {
  public:
    /**
     * @brief Expected to be executed whenever possible.
     * Time keeping is performed inside.
     */
    virtual void run() = 0;
};

class ReportHubInterface;

/**
 * @brief Interface for classes provide reports from somewhere.
 */
class ReportSourceInterface : public Runnable {
  public:
    /**
     * @brief Provides information on the expected output type
     *
     * @return ReportType  Either kMouse or kGamePad
     */
    virtual ReportType expected_report() = 0;

    /**
     * @brief Set the data sink which will be feeded with gathered report data
     *
     * @param target Input data sink
     */
    virtual void set_target(std::shared_ptr<ReportHubInterface> target) = 0;
};

/**
 * @brief Interface for classes that process HID reports from USB devices
 */
class HidHandlerInterface : public ReportSourceInterface {
  public:
    /**
     * @brief Called after the detection of a HID endpoint
     *
     * Allows further configuration.
     *
     * @param dev_addr  TinyUSB internal
     * @param instance  TinyUSB internal
     */
    virtual void setup_reception(int8_t dev_addr, uint8_t instance) = 0;

    /**
     * @brief Called after reception of HID reports.
     *
     * @param report The received report
     */
    virtual void process_report(std::span<const uint8_t> report) = 0;
};

/**
 * @brief Interface which processes joystick input
 */
class GamepadReportProcessor {
  public:
    /**
     * @brief Provides joystick button states to this implementation
     *
     * @param report  recent state
     */
    virtual void process_gamepad_report(GamepadReport &report) = 0;

    /**
     * @brief Expects to configure the GPIOs for joystick usage
     *
     * Expected to be relayed to the target \ref ControllerPortInterface
     * to configure the output circuitry for joystick output.
     */
    virtual void ensure_joystick_muxing() = 0;
};

/**
 * @brief Interface which processes mouse input
 */
class MouseReportProcessor {
  public:
    /**
     * @brief Provides mouse button states and incremental movement to this
     * implementation
     *
     * @param report  recent state and relative movement
     */
    virtual void process_mouse_report(MouseReport &report) = 0;

    /**
     * @brief Expects to configure the GPIOs for mouse usage
     *
     * Expected to be relayed to the target \ref ControllerPortInterface
     * to configure the output circuitry for mouse output.
     */
    virtual void ensure_mouse_muxing() = 0;
};

class RunnableMouseReportProcessor : public MouseReportProcessor, public Runnable {};

class RunnableGamepadReportProcessor : public GamepadReportProcessor, public Runnable {};

/**
 * @brief Interface which processes reports from both \ref MouseReportProcessor
 * and \ref GamepadReportProcessor.
 * Used for classes which behave on joystick vs. mouse behaviour.
 */
class ReportHubInterface : public MouseReportProcessor, public GamepadReportProcessor, public Runnable {

  public:
    /**
     * @brief Must be called with a source which sinks events into this object
     * Used by the implementation to check for the existence of the source.
     *
     * Example:
     * handler->set_target(primary_mouse_switcher_);
     * primary_mouse_switcher_->register_source(handler);
     * @param source  Object which uses this object as target
     */
    virtual void register_source(std::shared_ptr<ReportSourceInterface> source) = 0;
};

/**
 * Represents something which offers access to the outer world, driving an atari
 * type joystick port. Also has some helper functions for implementing C1351
 * signals.
 */

class ControllerPortInterface {
  public:
    /**
     * @brief Applies the full state to the output pins
     *
     * @param state State to apply
     */
    virtual void set_port_state(ControllerPortState &state) = 0;

    /**
     * @brief Returns the GPIO number which is used to drive the POT X pin
     *
     * @return uint RP2040 GPIO Number
     */
    virtual uint get_pot_x_drain_gpio() = 0;

    /**
     * @brief Returns the GPIO number which is used to drive the POT Y pin
     *
     * @return uint RP2040 GPIO Number
     */
    virtual uint get_pot_y_drain_gpio() = 0;

    /**
     * @brief Returns the GPIO number which is used to sense the low edge of the
     * POT Y signal. Required to detect the start of the SID ADC measurement
     * cycle
     *
     * @return uint RP2040 GPIO Number
     */
    virtual uint get_pot_y_sense_gpio() = 0;

    /**
     * @brief Apply the standard GPIO muxing and confguration
     */
    virtual void configure_gpios() = 0;

    /**
     * @brief Shall give textual representation of the implementation
     *
     * @return const char*  The name of the object to identify
     */
    virtual const char *get_name() = 0;

    /**
     * @brief Get the index of the port.
     * The indizes are starting at 0 and can
     * be used to uniquely identify a single port.
     *
     * Here it might be only 0 and 1.
     *
     * @return const size_t index
     */
    virtual size_t get_index() = 0;
};