#pragma once

#include <span>
#include <tuple>

class QuadratureEncoder {
  private:
    int32_t accumulator{0};

    uint8_t quadrature_state;
    const bool quadrature_a[4] = {0, 1, 1, 0};
    const bool quadrature_b[4] = {0, 0, 1, 1};

  public:
    void add_to_accumulator(int32_t increment) {
        this->accumulator += increment;
    }

    std::pair<bool, bool> update() {

        if (accumulator > 0) {
            accumulator--;
            quadrature_state--;
        } else if (accumulator < 0) {
            accumulator++;
            quadrature_state++;
        }

        bool a = quadrature_a[quadrature_state % 4];
        bool b = quadrature_b[quadrature_state % 4];
        return std::make_pair(a, b);
    }
};
