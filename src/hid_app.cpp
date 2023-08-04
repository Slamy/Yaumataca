
#include "bsp/board.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "hid.hpp"
#include <map>
#include <memory>

#define MAX_REPORT 4

// Each HID instance can has multiple reports
static struct {
    uint8_t report_count;
    tuh_hid_report_info_t report_info[MAX_REPORT];
    std ::unique_ptr<HidHandler> handler;
} hid_info[CFG_TUH_HID];

std::vector<HidHandlerBuilder *> HidHandlerBuilder::builders;

void hid_app_task(void) {
    for (auto &i : hid_info) {
        if (i.handler) {
            i.handler.get()->run();
        }
    }
}

//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use.
// tuh_hid_parse_report_descriptor() can be used to parse common/simple enough
// descriptor. Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE,
// it will be skipped therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const *desc_report, uint16_t desc_len) {
    (void)desc_report;
    (void)desc_len;
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    printf("HID device address = %d, instance = %d is mounted\r\n", dev_addr,
           instance);
    printf("VID = %04x, PID = %04x\r\n", vid, pid);

    // Interface protocol (hid_interface_protocol_enum_t)
    // const char *protocol_str[] = {"None", "Keyboard", "Mouse"};
    uint8_t itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    printf("HID Interface Protocol = %d\r\n", itf_protocol);

    hid_info[instance].report_count = tuh_hid_parse_report_descriptor(
        hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
    printf("HID has %u reports %d %d %d\r\n", hid_info[instance].report_count,
           hid_info[instance].report_info[0].report_id,
           hid_info[instance].report_info[0].usage,
           hid_info[instance].report_info[0].usage_page);

    // bool result = tuh_hid_set_protocol(dev_addr, instance,
    // HID_PROTOCOL_REPORT); printf("tuh_hid_set_protocol = %d\r\n", result);

    hid_info[instance].handler =
        HidHandlerBuilder::find(vid, pid, hid_info[instance].report_info);

    for (int i = 0; i < desc_len; i++) {
        printf("%02x ", desc_report[i]);
    }
    printf("\r\n");

    if (hid_info[instance].handler) {
        hid_info[instance].handler->setup_reception(dev_addr, instance);
    } else {
        // request to receive report
        // tuh_hid_report_received_cb() will be invoked when report is available
        if (!tuh_hid_receive_report(dev_addr, instance)) {
            printf("Error: cannot request to receive report\r\n");
        }
    }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    printf("HID device address = %d, instance = %d is unmounted\r\n", dev_addr,
           instance);
}

void print_generic_report(uint8_t const *d, uint16_t len) {
    printf("Report:");
    for (int i = 0; i < len; i++) {
        printf(" %02x", d[i]);
    }
    printf("\r\n");
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                uint8_t const *report, uint16_t len) {
    // print_generic_report(report, len);

    if (hid_info[instance].handler) {
        hid_info[instance].handler.get()->process_report(
            std::span(report, len));
    } else {
        print_generic_report(report, len);
    }
    // continue to request to receive report
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        printf("Error: cannot request to receive report\r\n");
    }
}

void tuh_hid_report_sent_cb(uint8_t dev_addr, uint8_t idx, uint8_t const *,
                            uint16_t len) {
    printf("tuh_hid_report_sent_cb %d %d %d\r\n", dev_addr, idx, len);
}

void tuh_hid_set_report_complete_cb(uint8_t dev_addr, uint8_t idx,
                                    uint8_t report_id, uint8_t report_type,
                                    uint16_t len) {

    printf("tuh_hid_set_report_complete_cb %d %d %d %d %d\r\n", dev_addr, idx,
           report_id, report_type, len);

    // request to receive report
    // tuh_hid_report_received_cb() will be invoked when report is available
    if (!tuh_hid_receive_report(dev_addr, idx)) {
        printf("Error: cannot request to receive report\r\n");
    }
}
