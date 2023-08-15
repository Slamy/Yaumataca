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

class DefaultHidHandler : public HidHandlerInterface {
  protected:
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
};

class HidHandlerBuilder {

  private:
    static std::vector<HidHandlerBuilder *> builders_;
    uint16_t vid_;
    uint16_t pid_;
    std::function<std::unique_ptr<HidHandlerInterface>()> make_;
    std::function<std::unique_ptr<HidHandlerInterface>(
        tuh_hid_report_info_t *info)>
        custom_matcher_;

    std::unique_ptr<HidHandlerInterface> matches(uint16_t vid, uint16_t pid) {
        std::unique_ptr<HidHandlerInterface> ptr;

        if (vid == vid_ && pid == pid_) {
            ptr = make_();
        }
        return ptr;
    }

  public:
    HidHandlerBuilder(
        uint16_t vid, uint16_t pid,
        std::function<std::unique_ptr<HidHandlerInterface>()> make,
        std::function<
            std::unique_ptr<HidHandlerInterface>(tuh_hid_report_info_t *info)>
            custom_matcher)
        : vid_(vid), pid_(pid), make_(make), custom_matcher_(custom_matcher) {
        builders_.push_back(this);
    }

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
