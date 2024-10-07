/**
 * @file bare_api.cpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2024-02-14
 *
 * @copyright Copyright (c) 2023
 *
 * Nearly everything here is from
 * tinyusb/examples/host/bare_api/src/main.c
 */

#include <cstdio>
#include <cstdlib>
#include <map>

#include "controller_port.hpp"
#include "global.hpp"
#include "handlers/bare_xbox_one.hpp"
#include "pico/stdlib.h"
#include "processors/pipeline.hpp"
#include "tusb.h"
#include "utility.h"

/// last mounted device descriptor
tusb_desc_device_t desc_device;

/// English (United States)
#define LANGUAGE_ID 0x0409

static void _convert_utf16le_to_utf8(const uint16_t *utf16, size_t utf16_len, uint8_t *utf8, size_t utf8_len) {
    // TODO: Check for runover.
    (void)utf8_len;
    // Get the UTF-16 length out of the data itself.

    for (size_t i = 0; i < utf16_len; i++) {
        uint16_t chr = utf16[i];
        if (chr < 0x80) {
            *utf8++ = chr & 0xffu;
        } else if (chr < 0x800) {
            *utf8++ = (uint8_t)(0xC0 | (chr >> 6 & 0x1F));
            *utf8++ = (uint8_t)(0x80 | (chr >> 0 & 0x3F));
        } else {
            // TODO: Verify surrogate.
            *utf8++ = (uint8_t)(0xE0 | (chr >> 12 & 0x0F));
            *utf8++ = (uint8_t)(0x80 | (chr >> 6 & 0x3F));
            *utf8++ = (uint8_t)(0x80 | (chr >> 0 & 0x3F));
        }
        // TODO: Handle UTF-16 code points that take two entries.
    }
}

// Count how many bytes a utf-16-le encoded string will take in utf-8.
static int _count_utf8_bytes(const uint16_t *buf, size_t len) {
    size_t total_bytes = 0;
    for (size_t i = 0; i < len; i++) {
        uint16_t chr = buf[i];
        if (chr < 0x80) {
            total_bytes += 1;
        } else if (chr < 0x800) {
            total_bytes += 2;
        } else {
            total_bytes += 3;
        }
        // TODO: Handle UTF-16 code points that take two entries.
    }
    return (int)total_bytes;
}
static void print_utf16(uint16_t *temp_buf, size_t buf_len) {
    if ((temp_buf[0] & 0xff) == 0)
        return; // empty
    size_t utf16_len = ((temp_buf[0] & 0xff) - 2) / sizeof(uint16_t);
    size_t utf8_len = (size_t)_count_utf8_bytes(temp_buf + 1, utf16_len);
    _convert_utf16le_to_utf8(temp_buf + 1, utf16_len, (uint8_t *)temp_buf, sizeof(uint16_t) * buf_len);
    ((uint8_t *)temp_buf)[utf8_len] = '\0';

    printf((char *)temp_buf);
}

/**
 * @brief Calculates actual size of Configuration descriptor
 *
 * @param desc_itf      Pointer to Configuration Descriptor
 * @param itf_count     Number of interfaces to expect
 * @param max_len       Length of buffer of desc_itf in bytes
 * @return uint16_t Length in byte
 */
static uint16_t count_interface_total_len(tusb_desc_interface_t const *desc_itf, uint8_t itf_count, uint16_t max_len) {
    uint8_t const *p_desc = (uint8_t const *)desc_itf;
    uint16_t len = 0;

    while (itf_count--) {
        // Next on interface desc
        len += tu_desc_len(desc_itf);
        p_desc = tu_desc_next(p_desc);

        while (len < max_len) {
            // return on IAD regardless of itf count
            if (tu_desc_type(p_desc) == TUSB_DESC_INTERFACE_ASSOCIATION)
                return len;

            if ((tu_desc_type(p_desc) == TUSB_DESC_INTERFACE) &&
                ((tusb_desc_interface_t const *)p_desc)->bAlternateSetting == 0) {
                break;
            }

            len += tu_desc_len(p_desc);
            p_desc = tu_desc_next(p_desc);
        }
    }

    return len;
}

/**
 * @brief Hash map with the TinyUSB device address as key
 *
 * and implementations of \ref ReportSourceInterface as value.
 * Used to store mounted vendor class devices for proper deletion
 * during unmount.
 */
std::map<uint8_t, std::shared_ptr<ReportSourceInterface>> bare_handlers;

/**
 * @brief Called for every found USB interface description that is off vendor class type
 *
 * @param daddr     TinyUSB device identifier
 * @param desc_itf  Pointer to interface descriptor
 * @param max_len   Length of the given interface descriptor
 */
static void open_vendor_interface(uint8_t daddr, tusb_desc_interface_t const *desc_itf, uint16_t max_len) {
    uint16_t vid, pid;

    tuh_vid_pid_get(daddr, &vid, &pid);
    // Xbox One Controller

    if ((vid == XBOX_VID1 || vid == XBOX_VID2 || vid == XBOX_VID3 || vid == XBOX_VID4 || vid == XBOX_VID5 || vid == XBOX_VID6) &&
                       (pid == XBOX_ONE_PID1 || pid == XBOX_ONE_PID2 || pid == XBOX_ONE_PID3 || pid == XBOX_ONE_PID4 ||
                        pid == XBOX_ONE_PID5 || pid == XBOX_ONE_PID6 || pid == XBOX_ONE_PID7 || pid == XBOX_ONE_PID8 ||
                        pid == XBOX_ONE_PID9 || pid == XBOX_ONE_PID10 || pid == XBOX_ONE_PID11 || pid == XBOX_ONE_PID12 || 
                        pid == XBOX_ONE_PID13 || pid == XBOX_ONE_PID14)){
        auto handler = std::make_shared<XboxOneHandler>();

        handler->open_vendor_interface(daddr, desc_itf, max_len);
        bare_handlers[daddr] = handler;
        gbl_pipeline->integrate_handler(handler);
    }
}

/**
 * @brief Parses USB device descriptor
 *
 * and calls \ref open_vendor_interface for the first vendor class interface found.
 * All others are ignored
 *
 * @param dev_addr  TinyUSB device identifier
 * @param desc_cfg  Pointer to USB configuration descriptor
 */
static void parse_config_descriptor(uint8_t dev_addr, tusb_desc_configuration_t const *desc_cfg) {
    uint8_t const *desc_end = ((uint8_t const *)desc_cfg) + tu_le16toh(desc_cfg->wTotalLength);
    uint8_t const *p_desc = tu_desc_next(desc_cfg);

    std::ignore = dev_addr;
    // parse each interfaces
    while (p_desc < desc_end) {
        uint8_t assoc_itf_count = 1;

        // Class will always starts with Interface Association (if any) and then Interface descriptor
        if (TUSB_DESC_INTERFACE_ASSOCIATION == tu_desc_type(p_desc)) {
            tusb_desc_interface_assoc_t const *desc_iad = (tusb_desc_interface_assoc_t const *)p_desc;
            assoc_itf_count = desc_iad->bInterfaceCount;

            p_desc = tu_desc_next(p_desc); // next to Interface
        }

        // must be interface from now
        if (TUSB_DESC_INTERFACE != tu_desc_type(p_desc))
            return;
        tusb_desc_interface_t const *desc_itf = (tusb_desc_interface_t const *)p_desc;

        uint16_t const drv_len = count_interface_total_len(desc_itf, assoc_itf_count, (uint16_t)(desc_end - p_desc));

        // probably corrupted descriptor
        if (drv_len < sizeof(tusb_desc_interface_t))
            return;

        // only open and listen to HID endpoint IN
        PRINTF("desc_itf->bInterfaceClass %d\r\n", desc_itf->bInterfaceClass);

        if (desc_itf->bInterfaceClass == TUSB_CLASS_VENDOR_SPECIFIC) {
            open_vendor_interface(dev_addr, desc_itf, drv_len);
            break;
        }

        // next Interface or IAD descriptor
        p_desc += drv_len;
    }
}

static void handle_device_descriptor(tuh_xfer_t *xfer) {
    if (XFER_RESULT_SUCCESS != xfer->result) {
        printf("Failed to get device descriptor\r\n");
        return;
    }

    uint8_t const daddr = xfer->daddr;

    printf("Device %u: ID %04x:%04x\r\n", daddr, desc_device.idVendor, desc_device.idProduct);
    printf("Device Descriptor:\r\n");
    printf("  bLength             %u\r\n", desc_device.bLength);
    printf("  bDescriptorType     %u\r\n", desc_device.bDescriptorType);
    printf("  bcdUSB              %04x\r\n", desc_device.bcdUSB);
    printf("  bDeviceClass        %u\r\n", desc_device.bDeviceClass);
    printf("  bDeviceSubClass     %u\r\n", desc_device.bDeviceSubClass);
    printf("  bDeviceProtocol     %u\r\n", desc_device.bDeviceProtocol);
    printf("  bMaxPacketSize0     %u\r\n", desc_device.bMaxPacketSize0);
    printf("  idVendor            0x%04x\r\n", desc_device.idVendor);
    printf("  idProduct           0x%04x\r\n", desc_device.idProduct);
    printf("  bcdDevice           %04x\r\n", desc_device.bcdDevice);

    // Get String descriptor using Sync API
    uint16_t temp_buf[128];

    printf("  iManufacturer       %u     ", desc_device.iManufacturer);
    if (XFER_RESULT_SUCCESS ==
        tuh_descriptor_get_manufacturer_string_sync(daddr, LANGUAGE_ID, temp_buf, sizeof(temp_buf))) {
        print_utf16(temp_buf, TU_ARRAY_SIZE(temp_buf));
    }
    printf("\r\n");

    printf("  iProduct            %u     ", desc_device.iProduct);
    if (XFER_RESULT_SUCCESS == tuh_descriptor_get_product_string_sync(daddr, LANGUAGE_ID, temp_buf, sizeof(temp_buf))) {
        print_utf16(temp_buf, TU_ARRAY_SIZE(temp_buf));
    }
    printf("\r\n");

    printf("  iSerialNumber       %u     ", desc_device.iSerialNumber);
    if (XFER_RESULT_SUCCESS == tuh_descriptor_get_serial_string_sync(daddr, LANGUAGE_ID, temp_buf, sizeof(temp_buf))) {
        print_utf16(temp_buf, TU_ARRAY_SIZE(temp_buf));
    }
    printf("\r\n");

    printf("  bNumConfigurations  %u\r\n", desc_device.bNumConfigurations);

    // Get configuration descriptor with sync API
    if (XFER_RESULT_SUCCESS == tuh_descriptor_get_configuration_sync(daddr, 0, temp_buf, sizeof(temp_buf))) {
        parse_config_descriptor(daddr, (tusb_desc_configuration_t *)temp_buf);
    }
}

/// Invoked when device is mounted (configured)
void tuh_mount_cb(uint8_t daddr) {
    printf("Device attached, address = %d\r\n", daddr);
    uint16_t vid, pid;

    tuh_vid_pid_get(daddr, &vid, &pid);

    PRINTF("HID device address = %d is mounted\n", daddr);
    PRINTF("VID = %04x, PID = %04x\n", vid, pid);

    // Xbox One Controller
    if (vid == 0x045e && pid == 0x0b12) {
        tuh_descriptor_get_device(daddr, &desc_device, 18, handle_device_descriptor, 0);
    }
}

/// Invoked when device is unmounted (bus reset/unplugged)
void tuh_umount_cb(uint8_t daddr) {
    printf("Device removed, address = %d\r\n", daddr);
    bare_handlers.erase(daddr);
}
