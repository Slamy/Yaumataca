#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include "tusb.h"

class HidHandler {
  public:
    virtual void process_report(std::span<const uint8_t> report) = 0;
    virtual void run(){};
    virtual void setup_reception(int8_t dev_addr, uint8_t instance) {
        // request to receive report
        // tuh_hid_report_received_cb() will be invoked when report is available
        if (!tuh_hid_receive_report(dev_addr, instance)) {
            printf("Error: cannot request to receive report\r\n");
        }
    }
};

class HidHandlerBuilder {

  private:
    static std::vector<HidHandlerBuilder *> builders;
    uint16_t vid_;
    uint16_t pid_;
    std::function<std::unique_ptr<HidHandler>()> make_;
    std::function<std::unique_ptr<HidHandler>(tuh_hid_report_info_t *info)>
        custom_matcher_;

    std::unique_ptr<HidHandler> matches(uint16_t vid, uint16_t pid) {
        std::unique_ptr<HidHandler> ptr;

        if (vid == vid_ && pid == pid_) {
            ptr = make_();
        }
        return ptr;
    }

  public:
    HidHandlerBuilder(
        uint16_t vid, uint16_t pid,
        std::function<std::unique_ptr<HidHandler>()> make,
        std::function<std::unique_ptr<HidHandler>(tuh_hid_report_info_t *info)>
            custom_matcher)
        : vid_(vid), pid_(pid), make_(make), custom_matcher_(custom_matcher) {
        builders.push_back(this);
    }

    static std::unique_ptr<HidHandler> find(uint16_t vid, uint16_t pid,
                                            tuh_hid_report_info_t *info) {
        std::unique_ptr<HidHandler> ptr;

        // Try to match first with VID and PID
        for (auto &i : builders) {
            ptr = i->matches(vid, pid);
            if (ptr) {
                return ptr;
            }
        }

        for (auto &i : builders) {

            if (i->custom_matcher_) {
                ptr = i->custom_matcher_(info);
                if (ptr) {
                    return ptr;
                }
            }
        }

        // Ok, then try with custom matchers

        return ptr;
    }
};

struct AmigaJoyPort {
    int fire;
    int secFire;
    int up, down, left, right;
};
