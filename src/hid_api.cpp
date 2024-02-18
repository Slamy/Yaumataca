/**
 * @file hid_api.cpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <map>
#include <memory>

#include "controller_port.hpp"
#include "default_hid_handler.hpp"
#include "global.hpp"
#include "hid_handler_builder.hpp"
#include "pico/stdlib.h"
#include "processors/pipeline.hpp"

/// Maximum number of reports to read from a single HID Report Descriptor
#define MAX_REPORT 4

// Each HID instance can has multiple reports
static struct {
    uint8_t report_count;
    tuh_hid_report_info_t report_info[MAX_REPORT];
    std::shared_ptr<HidHandlerInterface> handler;
} hid_info[CFG_TUH_HID];

void hid_app_task() {
    for (auto &i : hid_info) {
        if (i.handler)
            i.handler->run();
    }
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

/**
 * @brief Invoked when device with hid interface is mounted
 * Report descriptor is also available for use.
 * tuh_hid_parse_report_descriptor() can be used to parse common/simple enough
 * descriptor. Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE,
 * it will be skipped therefore report_desc = NULL, desc_len = 0
 *
 */
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len) {
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    PRINTF("HID device address = %d, instance = %d is mounted\n", dev_addr, instance);
    PRINTF("VID = %04x, PID = %04x\n", vid, pid);

    hid_info[instance].report_count =
        tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
    PRINTF("HID has %u reports %d %d %d\n", hid_info[instance].report_count,
           hid_info[instance].report_info[0].report_id, hid_info[instance].report_info[0].usage,
           hid_info[instance].report_info[0].usage_page);

    hid_info[instance].handler = HidHandlerBuilder::find(vid, pid, hid_info[instance].report_info);

    if (hid_info[instance].handler) {
        gbl_pipeline->integrate_handler(hid_info[instance].handler);
        hid_info[instance].handler->setup_reception(dev_addr, instance);
    } else {
        // request to receive report
        // tuh_hid_report_received_cb() will be invoked when report is
        // available
        if (!tuh_hid_receive_report(dev_addr, instance)) {
            PRINTF("Error: cannot request to receive report\n");
        }
    }
}

/**
 * @brief Invoked when device with hid interface is un-mounted
 *
 * @param dev_addr TinyUSB internal device identifier
 * @param instance TinyUSB internal endpoint identifier
 */
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    std::ignore = dev_addr;

    PRINTF("HID device address = %d, instance = %d is unmounted\n", dev_addr, instance);

    hid_info[instance].handler.reset();
}

/**
 * @brief Small helper function to print an array
 *
 * @param d     data
 * @param len   length in bytes
 */
void print_generic_report(uint8_t const *d, uint16_t len) {
    std::ignore = len;
    std::ignore = d;

    PRINTF("Report:");
    for (int i = 0; i < len; i++) {
        PRINTF(" %02x", d[i]);
    }
    PRINTF("\n");
}

/**
 * @brief Invoked when received report from device via interrupt endpoint
 *
 * @param dev_addr TinyUSB internal device identifier
 * @param instance TinyUSB internal endpoint identifier
 * @param report Raw report data
 * @param len Length of report in bytes
 */
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
    if (hid_info[instance].handler) {
        hid_info[instance].handler.get()->process_report(std::span(report, len));
    } else {
        print_generic_report(report, len);
    }

    // continue to request to receive report
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        PRINTF("Error: cannot request to receive report\n");
    }
}

/**
 * @brief Invoked when sent report to device successfully via interrupt endpoint
 *
 * @param dev_addr
 * @param idx
 * @param len
 */
void tuh_hid_report_sent_cb(uint8_t dev_addr, uint8_t idx, uint8_t const *, uint16_t len) {
    std::ignore = len;
    std::ignore = dev_addr;
    std::ignore = idx;

    PRINTF("tuh_hid_report_sent_cb %d %d %d\n", dev_addr, idx, len);
}

/**
 * @brief Invoked when Sent Report to device via either control endpoint
 * len = 0 indicate there is error in the transfer e.g stalled response
 *
 * @param dev_addr
 * @param idx
 * @param report_id
 * @param report_type
 * @param len
 */
void tuh_hid_set_report_complete_cb(uint8_t dev_addr, uint8_t idx, uint8_t report_id, uint8_t report_type,
                                    uint16_t len) {
    std::ignore = dev_addr;
    std::ignore = idx;
    std::ignore = report_id;
    std::ignore = report_type;
    std::ignore = len;

    PRINTF("tuh_hid_set_report_complete_cb %d %d %d %d %d\n", dev_addr, idx, report_id, report_type, len);
}

/**
 * @brief Invoked when Set Protocol request is complete
 *
 * @param dev_addr
 * @param idx
 * @param protocol
 */
void tuh_hid_set_protocol_complete_cb(uint8_t dev_addr, uint8_t idx, uint8_t protocol) {
    std::ignore = dev_addr;
    std::ignore = idx;
    std::ignore = protocol;

    PRINTF("tuh_hid_set_protocol_complete_cb %d %d   %d\n", dev_addr, idx, protocol);
}
