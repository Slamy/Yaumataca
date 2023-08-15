/** @file controller_port.hpp
 *
 */

#pragma once

#include <array>
#include <memory>

#include "bsp/board.h"
#include "pico/stdlib.h"
#include "processors/interfaces.hpp"
#include "tusb.h"
#include "utility.h"

// got singleton pattern from
// https://stackoverflow.com/questions/1008019/c-singleton-design-pattern

/**
 * @brief Representation of ownership of physical controller port.
 * There are always two. Cannot be constructed from the outside because of that.
 *
 */
class RightControllerPort : public ControllerPortInterface {
  private:
    RightControllerPort() {}

  public:
    RightControllerPort(RightControllerPort const &) = delete;
    void operator=(RightControllerPort const &) = delete;

    const char *get_name() override { return "Right/CP1/Mouse"; }

    static std::unique_ptr<RightControllerPort> getInstance() {
        static std::unique_ptr<RightControllerPort> instance =
            std::unique_ptr<RightControllerPort>(new RightControllerPort());
        return std::move(instance);
    }

    uint get_pot_x_drain_gpio() override { return 7; }
    uint get_pot_y_drain_gpio() override { return 11; }
    uint get_pot_y_sense_gpio() override { return 13; }

    void configure_gpios() override {
        const uint drain_pins[] = {7, 9, 10, 11, 12, 14, 15};

        const uint sense_pin = get_pot_y_sense_gpio();

        gpio_set_dir(sense_pin, GPIO_IN);
        gpio_set_pulls(sense_pin, true, false);
        gpio_set_input_hysteresis_enabled(sense_pin, 1);

        for (auto i : drain_pins) {
            gpio_init(i);
            gpio_set_dir(i, GPIO_OUT);
            gpio_put(i, 0);
        }
        PRINTF("GPIOs set for joystick mode on right port\n");
    }

    void set_port_state(ControllerPortState &state) override {
        gpio_put(7, state.fire2); // Also used as Pot X
        gpio_put(9, state.fire1);
        gpio_put(15, state.up);
        gpio_put(11, state.fire3); // Also used as Pot Y
        gpio_put(14, state.down);
        gpio_put(12, state.left);
        gpio_put(10, state.right);

        PRINTF("R %d%d%d%d %d%d%d\n", state.left, state.up, state.down,
               state.right, state.fire1, state.fire2, state.fire3);
    }
};

class LeftControllerPort : public ControllerPortInterface {

    LeftControllerPort() {}

  public:
    const char *get_name() override { return "Left/CP2/Joystick"; }

    LeftControllerPort(LeftControllerPort const &) = delete;
    void operator=(LeftControllerPort const &) = delete;

    static std::unique_ptr<LeftControllerPort> getInstance() {
        static std::unique_ptr<LeftControllerPort> instance =
            std::unique_ptr<LeftControllerPort>(new LeftControllerPort());
        return std::move(instance);
    }

    uint get_pot_x_drain_gpio() override { return 6; }
    uint get_pot_y_drain_gpio() override { return 5; }
    uint get_pot_y_sense_gpio() override { return 8; }

    void configure_gpios() override {
        const uint drain_pins[] = {0, 1, 2, 3, 4, 5, 6};

        const uint sense_pin = get_pot_y_sense_gpio();

        gpio_set_dir(sense_pin, GPIO_IN);
        gpio_set_pulls(sense_pin, true, false);
        gpio_set_input_hysteresis_enabled(sense_pin, 1);

        for (auto i : drain_pins) {
            gpio_init(i);
            gpio_set_dir(i, GPIO_OUT);
            gpio_put(i, 0);
        }

        PRINTF("GPIOs set for joystick mode on left port\n");
    }

    void set_port_state(ControllerPortState &state) {
        gpio_put(0, state.right);
        gpio_put(1, state.left);
        gpio_put(2, state.down);
        gpio_put(3, state.up);
        gpio_put(4, state.fire1);
        gpio_put(5, state.fire3); // Also used as Pot Y
        gpio_put(6, state.fire2); // Also used as Pot X

        PRINTF("L %d%d%d%d %d%d%d\n", state.left, state.up, state.down,
               state.right, state.fire1, state.fire2, state.fire3);
    }
};
