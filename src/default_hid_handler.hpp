/**
 * @file default_hid_handler.hpp
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
#include <functional>
#include <memory>
#include <span>
#include <variant>
#include <vector>

#include "processors/interfaces.hpp"
#include "utility.h"

/**
 * @brief Helper class to derive from.
 * Implements common functionality between other implementers of \ref
 * HidHandlerInterface to avoid boilerplate code.
 */
class DefaultHidHandler : public HidHandlerInterface {
  protected:
    /// @brief data sinkt to feed reports to
    std::shared_ptr<ReportHubInterface> target_;

  public:
    void set_target(std::shared_ptr<ReportHubInterface> target) override {
        target_ = target;
    }

    void setup_reception(int8_t dev_addr, uint8_t instance) override {
        // request to receive report
        // tuh_hid_report_received_cb() will be invoked when report is available
        if (!tuh_hid_receive_report(dev_addr, instance)) {
            PRINTF("Error: cannot request to receive report\n");
        }
    }

    void run() override{};
};
