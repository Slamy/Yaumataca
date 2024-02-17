/**
 * @file led_task.hpp
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
#include <cstdint>
#include <span>

#include <bsp/board_api.h>

/**
 * @brief Generates various blinking patterns
 *
 */
class LedPatternGenerator : public Runnable {
  private:
    /// @brief last written state of the LED. Nice for toggling
    bool current_state{false};

    /// 2 long blinks with an equally long pause
    static constexpr uint16_t blink_2long[] = {200, 200, 200, 50};
    /// 4 short blinks with long pauses
    static constexpr uint16_t blink_4short[] = {50, 200, 50, 200, 50, 200, 50, 50};
    /// 3 short blinks with long pauses
    static constexpr uint16_t blink_3short[] = {50, 250, 50, 250, 50, 50};
    /// looks like the character R is morsed
    static constexpr uint16_t blink_morse_r[] = {50, 250, 200, 250, 50, 50};
    /// 1 short flash
    static constexpr uint16_t blink_1short[] = {50, 10};

    /// @brief Collection of all blinking patterns
    std::span<const uint16_t> variants[5] = {
        std::span(blink_2long),  std::span(blink_4short),  std::span(blink_1short),
        std::span(blink_3short), std::span(blink_morse_r),
    };

    /// @brief currently selected blinking pattern
    std::span<const uint16_t> view_;

    /// @brief current position inside current the blinking pattern
    std::span<const uint16_t>::iterator it_;

    /// @brief absolute time in milliseconds when to advance the blinking
    /// pattern
    uint32_t wait_until_ms{0};

    /// @brief true if blinking is performed
    bool active_{false};

  public:
    /// @brief Possible pattern to perform
    enum Pattern {
        k2Long,
        k4Short,
        k1Short,
        k3Short,
        kMorseR,
    };
    LedPatternGenerator() {
    }

    /**
     * @brief Returns true if blinking pattern is active
     */
    bool active() {
        return active_;
    }

    /**
     * @brief Starts an LED blinking pattern
     *
     * @param index pattern to show
     */
    void set_pattern(enum Pattern index) {
        if (active_ == false) {
            view_ = variants[index];
            it_ = view_.begin();
            wait_until_ms = board_millis() + *it_;

            current_state = true;
            board_led_write(current_state);
            active_ = true;
        }
    }

    void run() override {
        if (active_) {
            if (board_millis() >= wait_until_ms) {
                current_state = !current_state;

                it_++;
                wait_until_ms = board_millis() + *it_;

                if (it_ == view_.end()) {
                    active_ = false;
                    current_state = false;
                }
                board_led_write(current_state);
            }
        }
    }
};