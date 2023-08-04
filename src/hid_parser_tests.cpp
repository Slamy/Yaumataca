

#include <cstdint>
#include <gtest/gtest.h>

#include "tusb.h"

class ReportParser {};

uint8_t impact_hid_report[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop Ctrls)
    0x09, 0x04,       // Usage (Joystick)
    0xA1, 0x01,       // Collection (Application)
    0x09, 0x01,       //   Usage (Pointer)
    0xA1, 0x00,       //   Collection (Physical)
    0x09, 0x30,       //     Usage (X)
    0x09, 0x31,       //     Usage (Y)
    0x09, 0x32,       //     Usage (Z)
    0x09, 0x35,       //     Usage (Rz)
    0x15, 0x00,       //     Logical Minimum (0)
    0x26, 0xFF, 0x00, //     Logical Maximum (255)
    0x35, 0x00,       //     Physical Minimum (0)
    0x46, 0xFF, 0x00, //     Physical Maximum (255)
    0x66, 0x00, 0x00, //     Unit (None)
    0x75, 0x08,       //     Report Size (8)
    0x95, 0x04,       //     Report Count (4)
    0x81, 0x02, //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                //     Null Position)
    0xC0,       //   End Collection
    0x09, 0x39, //   Usage (Hat switch)
    0x15, 0x01, //   Logical Minimum (1)
    0x25, 0x08, //   Logical Maximum (8)
    0x35, 0x00, //   Physical Minimum (0)
    0x46, 0x3B, 0x01, //   Physical Maximum (315)
    0x65, 0x14,       //   Unit (System: English Rotation, Length: Centimeter)
    0x75, 0x04,       //   Report Size (4)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                //   Position)
    0x05, 0x09, //   Usage Page (Button)
    0x19, 0x01, //   Usage Minimum (0x01)
    0x29, 0x0C, //   Usage Maximum (0x0C)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //   Logical Maximum (1)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x0C, //   Report Count (12)
    0x55, 0x00, //   Unit Exponent (0)
    0x65, 0x00, //   Unit (None)
    0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                //   Position)
    0x05, 0x08, //   Usage Page (LEDs)
    0x09, 0x43, //   Usage (Slow Blink On Time)
    0x15, 0x00, //   Logical Minimum (0)
    0x26, 0xFF, 0x00, //   Logical Maximum (255)
    0x35, 0x00,       //   Physical Minimum (0)
    0x46, 0xFF, 0x00, //   Physical Maximum (255)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x04,       //   Report Count (4)
    0x91, 0x82, //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
                //   Position,Volatile)
    0x55, 0x00, //   Unit Exponent (0)
    0x65, 0x00, //   Unit (None)
    0x55, 0x00, //   Unit Exponent (0)
    0x65, 0x00, //   Unit (None)
    0x55, 0x00, //   Unit Exponent (0)
    0xC0,       // End Collection
};

uint8_t tuh_hid_parse_report_descriptor(uint8_t const *desc_report,
                                        uint16_t desc_len) {

    uint8_t arr_count = 1; // TODO really a good idea?

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
    //  uint8_t ri_report_count = 0;
    //  uint8_t ri_report_size = 0;

    uint8_t ri_collection_depth = 0;

    while (desc_len && report_num < arr_count) {
        header.byte = *desc_report++;
        desc_len--;

        uint8_t const tag = header.tag;
        uint8_t const type = header.type;
        uint8_t const size = header.size;

        uint8_t const data8 = desc_report[0];

        printf("tag = %d, type = %d, size = %d, data = ", tag, type, size);
        for (uint32_t i = 0; i < size; i++)
            printf("%02X ", desc_report[i]);
        printf("\r\n");

        switch (type) {
        case RI_TYPE_MAIN:
            switch (tag) {
            case RI_MAIN_INPUT:
                break;
            case RI_MAIN_OUTPUT:
                break;
            case RI_MAIN_FEATURE:
                break;

            case RI_MAIN_COLLECTION:
                ri_collection_depth++;
                break;

            case RI_MAIN_COLLECTION_END:
                ri_collection_depth--;
                if (ri_collection_depth == 0) {
                    info++;
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
                if (ri_collection_depth == 0)
                    memcpy(&info->usage_page, desc_report, size);
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
                info->report_id = data8;
                break;

            case RI_GLOBAL_REPORT_SIZE:
                //            ri_report_size = data8;
                break;

            case RI_GLOBAL_REPORT_COUNT:
                //            ri_report_count = data8;
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
            case RI_LOCAL_USAGE:
                // only take in account the "usage" before starting REPORT ID
                if (ri_collection_depth == 0)
                    info->usage = data8;
                break;

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

    for (uint8_t i = 0; i < report_num; i++) {
        info = report_info_arr + i;
        printf("%u: id = %u, usage_page = %u, usage = %u\r\n", i,
               info->report_id, info->usage_page, info->usage);
    }

    return report_num;
}

TEST(myfunctions, add) { GTEST_ASSERT_EQ(add(10, 22), 32); }