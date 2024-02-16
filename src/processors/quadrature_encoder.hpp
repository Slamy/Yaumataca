/**
 * @file quadrature_encoder.hpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <span>
#include <tuple>

/**
 * @brief Simulates quadrature encoder
 * Accumulates movements over time. No movement is lost.
 */
class QuadratureEncoder {
  private:
    /// @brief Relative movement still to perform
    int32_t accumulator_{0};

    /// @brief Current phase position in the LUTs. Ranges 0-3
    uint8_t out_state_{0};

    /// @brief Just a square wave
    const bool lut_a[4] = {0, 1, 1, 0};

    /// @brief A square wave which is 90° shifted to \ref lut_a
    const bool lut_b[4] = {0, 0, 1, 1};

  public:
    /**
     * @brief Feed relative movement data
     *
     * @param increment movement
     */
    void add_to_accumulator(int32_t increment) {
        this->accumulator_ += increment;
    }

    /**
     * @brief Returns current movement accumulator
     *
     * @return int32_t Movement to perform in the future
     */
    int32_t accumulator() {
        return accumulator_;
    }

    /**
     * @brief Step quadrature signals into direction of accumulator
     * Returns pair of signals which are always 90° apart
     *
     * @return std::pair<bool, bool>    new state after step
     */
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
