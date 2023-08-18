/**
 * @file hid_handler.hpp
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
#include "tusb.h"
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

/**
 * @brief Builder class which collects all registered HID drivers
 * Provides implementations of \ref HidHandlerInterface when confronted
 * with VID/PID or HID reports.
 */
class HidHandlerBuilder {
  private:
    /// @brief All registered builders to match against
    static std::vector<HidHandlerBuilder *> builders_;

    /// @brief Vendor ID to match against
    uint16_t vid_;
    /// @brief Product ID to match against
    uint16_t pid_;
    /// @brief Lambda function to construct the Handler. Can be nullptr.
    std::function<std::unique_ptr<HidHandlerInterface>()> make_;
    /// @brief Lambda function to check for a matching handler via hid report
    /// info
    std::function<std::unique_ptr<HidHandlerInterface>(
        tuh_hid_report_info_t *info)>
        custom_matcher_;

    /**
     * @brief Checks if VID and PID do match. Returns implementation if
     * positive.
     *
     * @param vid   Vendor ID of the newly attached device
     * @param pid   Product ID of the newly attached device
     * @return std::unique_ptr<HidHandlerInterface> Matching implementation or
     * nullptr
     */
    std::unique_ptr<HidHandlerInterface> matches(uint16_t vid, uint16_t pid) {
        std::unique_ptr<HidHandlerInterface> ptr;

        if (vid == vid_ && pid == pid_) {
            ptr = make_();
        }
        return ptr;
    }

  public:
    /**
     * @brief Construct a new Hid Handler Builder for a driver
     *
     * @param vid   Vendor ID of the device
     * @param pid   Product ID of the device
     * @param make  Lambda function to construct the Handler. Can be nullptr if
     * VID and PID don't matter
     * @param custom_matcher    Lambda function to check for a matching handler
     * via hid report info
     */
    HidHandlerBuilder(
        uint16_t vid, uint16_t pid,
        std::function<std::unique_ptr<HidHandlerInterface>()> make,
        std::function<
            std::unique_ptr<HidHandlerInterface>(tuh_hid_report_info_t *info)>
            custom_matcher)
        : vid_(vid), pid_(pid), make_(make), custom_matcher_(custom_matcher) {
        builders_.push_back(this);
    }

    /**
     * @brief Trys to find a fitting handler for the parameters
     *
     * It is first tried to use the VID/PID. If no match is found,
     * the list of custom matchers are used.
     *
     * @param vid Vendor ID of the newly attached device
     * @param pid Product ID of the newly attached device
     * @param info Additional HID Report info
     * @return std::unique_ptr<HidHandlerInterface>
     */
    static std::unique_ptr<HidHandlerInterface>
    find(uint16_t vid, uint16_t pid, tuh_hid_report_info_t *info) {
        std::unique_ptr<HidHandlerInterface> ptr;

        // Try to match first with VID and PID
        for (auto &i : builders_) {
            ptr = i->matches(vid, pid);
            if (ptr) {
                return ptr;
            }
        }

        // Ok, then try with custom matchers
        for (auto &i : builders_) {

            if (i->custom_matcher_) {
                ptr = i->custom_matcher_(info);
                if (ptr) {
                    return ptr;
                }
            }
        }

        return ptr;
    }
};
