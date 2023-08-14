#pragma once

#include "processors/interfaces.hpp"
#include <cstdint>
#include <span>

class LedPatternGenerator : public Runnable {
  private:
    bool current_state{false};

    static constexpr uint16_t blink_2long[] = {200, 200, 200, 50};
    static constexpr uint16_t blink_4short[] = {50, 200, 50, 200,
                                                50, 200, 50, 50};
    static constexpr uint16_t blink_3short[] = {50, 250, 50, 250, 50, 50};
    static constexpr uint16_t blink_morse_r[] = {50, 250, 200, 250, 50, 50};
    static constexpr uint16_t blink_1short[] = {50, 10};

    std::span<const uint16_t> variants[5] = {
        std::span(blink_2long),   std::span(blink_4short),
        std::span(blink_1short),  std::span(blink_3short),
        std::span(blink_morse_r),
    };

    std::span<const uint16_t> view_;
    std::span<const uint16_t>::iterator it_;

    uint32_t wait_until_ms{0};
    bool active_{false};

  public:
    enum Pattern {
        k2Long,
        k4Short,
        k1Short,
        k3Short,
        kMorseR,
    };
    LedPatternGenerator() {}

    bool active() { return active_; }

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