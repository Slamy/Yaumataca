#pragma once

#include <span>
#include <tuple>

class QuadratureEncoder {
  private:
    int32_t accumulator_{0};
    uint8_t out_state_{0};
    const bool lut_a[4] = {0, 1, 1, 0};
    const bool lut_b[4] = {0, 0, 1, 1};

  public:
    void add_to_accumulator(int32_t increment) {
        this->accumulator_ += increment;
    }

    int32_t accumulator() { return accumulator_; }

    std::pair<bool, bool> update() {
        if (accumulator_ > 0) {
            accumulator_--;
            out_state_--;
        } else if (accumulator_ < 0) {
            accumulator_++;
            out_state_++;
        }

        bool out_a = lut_a[out_state_ % 4];
        bool out_b = lut_b[out_state_ % 4];
        return std::make_pair(out_a, out_b);
    }
};
