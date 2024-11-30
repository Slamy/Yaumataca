/**
 * @file hid_mouse.cpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <numeric>
#include <queue>

#include "default_hid_handler.hpp"
#include "field_extractor.hpp"
#include "hid_handler_builder.hpp"

/**
 * @brief Generic handler of USB HID reports for mouses
 * Will try to force the mouse into report mode to support wheel movement
 */
class MouseReportHandler : public DefaultHidHandler {
  private:
    FieldExtractor get_x;       ///< extracts relative X movement from report
    FieldExtractor get_y;       ///< extracts relative Y movement from report
    FieldExtractor get_wheel;   ///< extracts relative wheel movement from report
    FieldExtractor get_buttons; ///< extracts button flags from report
    /// Some HID have multiple reports. If it has, we need to filter
    /// out the right one.
    uint8_t expected_report_id_{0};

    /// HID Report Descriptor is usable for this application
    bool hid_report_desc_valid_{false};

  public:
    void parse_hid_report_descriptor(uint8_t const *desc_report, uint16_t desc_len) override {
        PRINTF("HID Descriptor:");
        for (size_t i = 0; i < desc_len; i++) {
            PRINTF(" %02x", desc_report[i]);
        }
        PRINTF("\n");

        hid_report_desc_valid_ = false;

        // Report Item 6.2.2.2 USB HID 1.11
        union TU_ATTR_PACKED {
            uint8_t byte;
            struct TU_ATTR_PACKED {
                uint8_t size : 2;
                uint8_t type : 2;
                uint8_t tag : 4;
            };
        } header;

        uint8_t report_num = 0;

        // current parsed report count & size from descriptor
        uint16_t ri_report_count = 0;
        uint16_t ri_report_size = 0;

        size_t bit_offset = 0;

        uint8_t ri_collection_depth = 0;

        std::queue<uint8_t> usages_level2;

        uint8_t usage0{0};
        uint8_t usage1{0};

        while (desc_len) {
            header.byte = *desc_report++;
            desc_len--;

            uint8_t const tag = header.tag;
            uint8_t const type = header.type;
            uint8_t const size = header.size;

            uint8_t const data8 = desc_report[0];

            switch (type) {
            case RI_TYPE_MAIN:
                switch (tag) {
                case RI_MAIN_INPUT: {

                    if (usage0 == HID_USAGE_DESKTOP_MOUSE && usage1 == HID_USAGE_DESKTOP_POINTER) {
                        bool is_relative = (data8 & HID_RELATIVE) != 0;
                        bool is_const = (data8 & HID_CONSTANT) != 0;

                        if (is_const) {
                            PRINTF("IGNORE!\n");
                            bit_offset += ri_report_size * ri_report_count;

                        } else {
                            if (is_relative) {
                                PRINTF("RI_MAIN_INPUT REL %d %d %x %d\n", ri_report_count, ri_report_size, data8,
                                       bit_offset);

                                if (usages_level2.size() != ri_report_count) {
                                    PRINTF("Report count not correct!\n");
                                    return;
                                }

                                while (usages_level2.empty() == false) {
                                    uint8_t usage = usages_level2.front();
                                    usages_level2.pop();

                                    switch (usage) {
                                    case HID_USAGE_DESKTOP_X:
                                        printf("* X Axis is at %d with a width of %d\n", bit_offset, ri_report_size);
                                        get_x.configure(bit_offset, ri_report_size, true);
                                        break;
                                    case HID_USAGE_DESKTOP_Y:
                                        printf("* Y Axis is at %d with a width of %d\n", bit_offset, ri_report_size);
                                        get_y.configure(bit_offset, ri_report_size, true);
                                        break;
                                    case HID_USAGE_DESKTOP_WHEEL:
                                        printf("* Wheel Axis is at %d with a width of %d\n", bit_offset,
                                               ri_report_size);
                                        get_wheel.configure(bit_offset, ri_report_size, true);
                                        break;
                                    }
                                    bit_offset += ri_report_size;
                                }

                            } else {
                                PRINTF("RI_MAIN_INPUT ABS %d %d %x %d\n", ri_report_count, ri_report_size, data8,
                                       bit_offset);

                                // Yes, report_count is correct.
                                // Buttons are bools
                                printf("* Buttons are at %d with a width of %d\n", bit_offset, ri_report_count);
                                get_buttons.configure(bit_offset, ri_report_count, false);

                                bit_offset += ri_report_size * ri_report_count;
                            }
                        }
                    }

                    break;
                }
                case RI_MAIN_OUTPUT:
                    break;
                case RI_MAIN_FEATURE:
                    break;

                case RI_MAIN_COLLECTION:

                    if (ri_collection_depth == 0)
                        bit_offset = 0;

                    ri_collection_depth++;
                    break;

                case RI_MAIN_COLLECTION_END:
                    ri_collection_depth--;
                    if (ri_collection_depth == 0) {
                        report_num++;
                    }
                    break;

                default:
                    break;
                }
                break;

            case RI_TYPE_GLOBAL:
                switch (tag) {
                case RI_GLOBAL_USAGE_PAGE:
                    // only take in account the "usage page" before REPORT ID
                    if (ri_collection_depth == 0) {
                    }

                    for (int i = 0; i < ri_collection_depth; i++)
                        PRINTF("  ");
                    PRINTF("RI_GLOBAL_USAGE_PAGE %02x\r\n", data8);
                    switch (data8) {
                    case HID_USAGE_PAGE_BUTTON:
                        PRINTF("BUTTON\n");
                        break;
                    }

                    break;

                case RI_GLOBAL_LOGICAL_MIN:
                    break;
                case RI_GLOBAL_LOGICAL_MAX:
                    break;
                case RI_GLOBAL_PHYSICAL_MIN:
                    break;
                case RI_GLOBAL_PHYSICAL_MAX:
                    break;

                case RI_GLOBAL_REPORT_ID:
                    bit_offset += 8;

                    for (int i = 0; i < ri_collection_depth; i++)
                        PRINTF("  ");

                    PRINTF("RI_GLOBAL_REPORT_ID %02x\r\n", data8);
                    if (usage0 == HID_USAGE_DESKTOP_MOUSE)
                        expected_report_id_ = data8;

                    break;

                case RI_GLOBAL_REPORT_SIZE:
                    ri_report_size = data8;
                    for (int i = 0; i < ri_collection_depth; i++)
                        PRINTF("  ");
                    PRINTF("RI_GLOBAL_REPORT_SIZE %d\r\n", data8);

                    break;

                case RI_GLOBAL_REPORT_COUNT:
                    ri_report_count = data8;
                    for (int i = 0; i < ri_collection_depth; i++)
                        PRINTF("  ");
                    PRINTF("RI_GLOBAL_REPORT_COUNT %02x\r\n", data8);

                    break;

                case RI_GLOBAL_UNIT_EXPONENT:
                    break;
                case RI_GLOBAL_UNIT:
                    break;
                case RI_GLOBAL_PUSH:
                    break;
                case RI_GLOBAL_POP:
                    break;

                default:
                    break;
                }
                break;

            case RI_TYPE_LOCAL:
                switch (tag) {
                case RI_LOCAL_USAGE: {
                    // only take in account the "usage" before starting REPORT ID

                    for (int i = 0; i < ri_collection_depth; i++)
                        PRINTF("  ");

                    const char *text{""};
                    std::ignore = text;
                    if (ri_collection_depth == 2) {
                        usages_level2.push(data8);

                        switch (data8) {
                        case HID_USAGE_DESKTOP_X:
                            text = "X";
                            break;
                        case HID_USAGE_DESKTOP_Y:
                            text = "Y";
                            break;
                        case HID_USAGE_DESKTOP_WHEEL:
                            text = "WHEEL";
                            break;
                        }
                    } else if (ri_collection_depth == 0) {
                        usage0 = data8;
                    } else if (ri_collection_depth == 1) {
                        usage1 = data8;
                    }

                    PRINTF("RI_LOCAL_USAGE %02x %s\r\n", data8, text);

                    break;
                }
                case RI_LOCAL_USAGE_MIN:
                    break;
                case RI_LOCAL_USAGE_MAX:
                    break;
                case RI_LOCAL_DESIGNATOR_INDEX:
                    break;
                case RI_LOCAL_DESIGNATOR_MIN:
                    break;
                case RI_LOCAL_DESIGNATOR_MAX:
                    break;
                case RI_LOCAL_STRING_INDEX:
                    break;
                case RI_LOCAL_STRING_MIN:
                    break;
                case RI_LOCAL_STRING_MAX:
                    break;
                case RI_LOCAL_DELIMITER:
                    break;
                default:
                    break;
                }
                break;

            // error
            default:
                break;
            }

            desc_report += size;
            desc_len -= size;
        }

        hid_report_desc_valid_ = true;
        PRINTF("Use report mode!\n");
    }

    void process_report(std::span<const uint8_t> report) override {
#if 0
        PRINTF("Mouse:");
        for (uint8_t i : report) {
            PRINTF(" %02x", i);
        }
        PRINTF("\n");
#endif
        MouseReport mouse_report;

        if (hid_report_desc_valid_) {
            if (expected_report_id_ > 0 && expected_report_id_ != report[0]) {
                PRINTF("DISCARD!\n");
                return;
            }

            mouse_report.relx = saturating_cast(get_x.extract(report.data(), report.size()));
            mouse_report.rely = saturating_cast(get_y.extract(report.data(), report.size()));
            mouse_report.wheel = saturating_cast(get_wheel.extract(report.data(), report.size()));
            mouse_report.button_pressed = static_cast<uint8_t>(get_buttons.extract(report.data(), report.size()));

        } else {
            mouse_report = *reinterpret_cast<const struct MouseReport *>(report.data());
        }

        if (target_) {
            PRINTF("Mouse X:%d Y:%d W:%d  %d %d %d   %x\n", mouse_report.relx, mouse_report.rely, mouse_report.wheel,
                   mouse_report.left, mouse_report.middle, mouse_report.right, mouse_report.button_pressed);
            target_->process_mouse_report(mouse_report);
        }
    }

    void setup_reception(int8_t dev_addr, uint8_t instance) override {
        if (hid_report_desc_valid_) {
            bool result = tuh_hid_set_protocol(dev_addr, instance, HID_PROTOCOL_REPORT);
            PRINTF("tuh_hid_set_protocol = %d\n", result);
            std::ignore = result;
        }

        if (!tuh_hid_receive_report(dev_addr, instance)) {
            PRINTF("Error: cannot request to receive report\n");
        }
    }

    ReportType expected_report() override {
        return kMouse;
    }
};

static HidHandlerBuilder builder(0, 0, nullptr, [](tuh_hid_report_info_t *info) {
    if (info->usage == HID_USAGE_DESKTOP_MOUSE && info->usage_page == HID_USAGE_PAGE_DESKTOP) {
        return std::make_unique<MouseReportHandler>();
    } else {
        return std::unique_ptr<MouseReportHandler>(nullptr);
    }
});
