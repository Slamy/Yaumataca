#include "hid_handler.hpp"

class MouseReportHandler : public DefaultHidHandler {

  public:
    void process_report(std::span<const uint8_t> report) override {

#if 0
        printf("Mouse:");
        for (uint8_t i : report) {
            printf(" %02x", i);
        }
        printf("\n");
#endif
        auto mouse_report =
            *reinterpret_cast<const struct MouseReport *>(report.data());

        if (target_) {
            target_->process_mouse_report(mouse_report);
        }
    }

    void setup_reception(int8_t dev_addr, uint8_t instance) override {
        bool result =
            tuh_hid_set_protocol(dev_addr, instance, HID_PROTOCOL_REPORT);
        printf("tuh_hid_set_protocol = %d\n", result);
    }

    ReportType expected_report() override { return kMouse; }
};

static HidHandlerBuilder
    builder(0, 0, nullptr, [](tuh_hid_report_info_t *info) {
        if (info->usage == HID_USAGE_DESKTOP_MOUSE &&
            info->usage_page == HID_USAGE_PAGE_DESKTOP) {
            return std::make_unique<MouseReportHandler>();
        } else {
            return std::unique_ptr<MouseReportHandler>(nullptr);
        }
    });
