/**
 * @file bare_xbox360_wireless.cpp
 * @author André Zeps, with help from quadflyer8
 * @brief
 * @version 0.1
 * @date 2024-12-30
 *
 * @copyright Copyright (c) 2024
 *
 */

// inspired by
// https://github.com/felis/USB_Host_Shield_2.0/blob/master/XBOXRECV.cpp

#include "bare_xbox360_wireless.hpp"
#include "global.hpp"

struct __attribute__((packed)) Xbox360WirelessButtonData {
    uint8_t type1; // if 0x08, connection status
    uint8_t type2; // if 0x01, button status
    uint8_t reserved1;
    uint8_t status1; // ?
    uint8_t status2; // ?
    uint8_t reserved2;

    // Byte 6
    bool dpad_up : 1;
    bool dpad_down : 1;
    bool dpad_left : 1;
    bool dpad_right : 1;

    bool start : 1;
    bool back : 1;
    bool left_stick_button : 1;
    bool right_stick_button : 1;

    // Byte 7
    bool trigger_left : 1;
    bool trigger_right : 1;
    bool guide : 1; // big green X
    bool sync : 1;

    bool a : 1;
    bool b : 1;
    bool x : 1;
    bool y : 1;

    // Byte 8, 9
    uint8_t analog_trigger_left;  // 0 when IDLE, 0xff when fully pressed
    uint8_t analog_trigger_right; // 0 when IDLE, 0xff when fully pressed

    // Byte 10, 11, 12, 13
    int16_t stick_left_x;  // -INT16MAX .. center 0 .. +INT16MAX
    int16_t stick_left_y;  // -INT16MAX .. center 0 .. +INT16MAX
    int16_t stick_right_x; // -INT16MAX .. center 0 .. +INT16MAX
    int16_t stick_right_y; // -INT16MAX .. center 0 .. +INT16MAX
};

static void c_report_received(tuh_xfer_t *xfer);

static void configure_finished_received(tuh_xfer_t *xfer) {
    std::ignore = xfer;
    PRINTF("configure_finished_received %d\r\n", xfer->result);
}

void Xbox360WirelessReceiverHandler::report_received(tuh_xfer_t *xfer) {
    auto obj = reinterpret_cast<Xbox360WirelessReceiverHandler::WirelessGamepadInstance *>(xfer->user_data);
    int index = obj->id_;
    uint8_t *buffer = user_data[index].buf_in_.data();

    if (xfer->result == XFER_RESULT_SUCCESS) {

        static constexpr int16_t kAnalogThreshold{16000};
        static constexpr uint8_t kTypeButtonData{0x01};
        static constexpr uint8_t kConnectionStatus{0x08};

        auto dat = reinterpret_cast<const Xbox360WirelessButtonData *>(buffer);

        if (dat->type1 == kConnectionStatus && dat->type2 == 0x80) {
            PRINTF("Connected %d\n", index);
            obj->report_proxy_ = std::make_shared<ReportProxy>();
            gbl_pipeline->integrate_handler(obj->report_proxy_);

            PRINTF("Send to %x\n", obj->xfer_out_.ep_addr);

            obj->buf_out_[0] = 0x00;
            obj->buf_out_[1] = 0x00;
            obj->buf_out_[2] = 0x08;
            obj->buf_out_[3] = index ? 0x43 : 0x042; // LED2 for second controller, LED1 for first controller

            // submit transfer for this EP
            bool result = tuh_edpt_xfer(&obj->xfer_out_);
            std::ignore = result; // TODO proper error handling

            PRINTF("out %d\r\n", result);
        }
        if (dat->type1 == kConnectionStatus && dat->type2 == 0x00) {
            PRINTF("Disconnected %d\n", index);
            obj->report_proxy_.reset();
        }

        if (dat->type1 == 0x00 && dat->type2 == kTypeButtonData) {
            PRINTF("Report @%d: ", index);
            for (uint32_t i = 0; i < xfer->actual_len; i++) {
                PRINTF(" %02x", buffer[i]);
            }
            PRINTF("\n");
            /*
            PRINTF("Xbox360W: %d%d%d%d %d%d%d%d %d %d\r\n", dat->dpad_down, dat->dpad_left, dat->dpad_right,
                   dat->dpad_up, dat->x, dat->y, dat->b, dat->a, dat->stick_left_x, dat->stick_left_y);
            */

            /*PRINTF("Xbox360W: %d %d %d %d\n", dat->stick_right_x, dat->stick_right_y, dat->analog_trigger_left,
                   dat->analog_trigger_right);*/

            /*
            PRINTF("Xbox360W: TL:%d TR:%d Sync:%d Guide:%d LS:%d RS:%d Back:%d Start:%d\r\n", dat->trigger_left,
                   dat->trigger_right, dat->sync, dat->guide, dat->left_stick_button, dat->right_stick_button,
                   dat->back, dat->start);*/

            GamepadReport aj;

            aj.left = dat->dpad_left || dat->stick_left_x < (-kAnalogThreshold);
            aj.down = dat->dpad_down || dat->stick_left_y < (-kAnalogThreshold);
            aj.right = dat->dpad_right || dat->stick_left_x > (+kAnalogThreshold);
            aj.up = dat->dpad_up || dat->stick_left_y > (+kAnalogThreshold);

            aj.fire = dat->x || dat->b;
            aj.sec_fire = dat->a;
            aj.third_fire = dat->trigger_left;
            aj.auto_fire = dat->y;

            aj.joystick_swap = dat->back;

            if (obj->report_proxy_->target_) {
                obj->report_proxy_->target_->process_gamepad_report(aj);
            }
        }
    }

    // continue to submit transfer, with updated buffer
    // other field remain the same
    xfer->buflen = 64;
    xfer->buffer = buffer;

    tuh_edpt_xfer(xfer);
}

bool Xbox360WirelessReceiverHandler::open_input_endpoint(uint8_t daddr, tusb_desc_endpoint_t const *desc_ep,
                                                         int index) {
    if (!tuh_edpt_open(daddr, desc_ep)) {
        PRINTF("tuh_edpt_open failed!\n");
        return false;
    }
    struct WirelessGamepadInstance *ud = &user_data[index];

    ud->xfer_in_ = {.daddr = daddr,
                    .ep_addr = desc_ep->bEndpointAddress,
                    .reserved2 = 0,
                    .result = XFER_RESULT_SUCCESS,
                    .actual_len = 0,
                    .buflen = ud->buf_in_.size(),
                    .buffer = ud->buf_in_.data(),
                    .complete_cb = c_report_received,
                    .user_data = reinterpret_cast<uintptr_t>(ud)};

    PRINTF("Listen to [dev %u: ep %02x]\r\n", daddr, desc_ep->bEndpointAddress);

    // submit transfer for this EP
    bool result = tuh_edpt_xfer(&ud->xfer_in_);
    std::ignore = result; // TODO proper error handling

    PRINTF("in %d\r\n", result);

    // Prepare corresponding output endpoint to set the LEDs
    tusb_desc_endpoint_t output_endp;
    output_endp = *desc_ep;
    output_endp.bEndpointAddress &= 0x7f;

    if (!tuh_edpt_open(daddr, &output_endp)) {
        PRINTF("tuh_edpt_open failed!\n");
        return false;
    }

    ud->xfer_out_ = {.daddr = daddr,
                     .ep_addr = output_endp.bEndpointAddress,
                     .reserved2 = 0,
                     .result = XFER_RESULT_SUCCESS,
                     .actual_len = 4,
                     .buflen = ud->buf_out_.size(),
                     .buffer = ud->buf_out_.data(),
                     .complete_cb = configure_finished_received,
                     .user_data = reinterpret_cast<uintptr_t>(ud)};

    PRINTF("Prepared output [dev %u: ep %02x]\r\n", daddr, output_endp.bEndpointAddress);

    return true;
}

void Xbox360WirelessReceiverHandler::open_vendor_interface(uint8_t daddr, tusb_desc_interface_t const *desc_itf,
                                                           uint16_t max_len) {
    // len = interface + hid + n*endpoints
    uint16_t const drv_len = (uint16_t)(sizeof(tusb_desc_interface_t) + sizeof(tusb_hid_descriptor_hid_t) +
                                        desc_itf->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

    // corrupted descriptor
    if (max_len < drv_len)
        return;

    printf("Num Endpoints %d\n", desc_itf->bNumEndpoints);
    uint8_t const *p_desc = (uint8_t const *)desc_itf;

    // Endpoint descriptor
    p_desc = tu_desc_next(p_desc);
    tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;

    user_data[0].handler_ = this;
    user_data[0].id_ = 0;
    user_data[1].handler_ = this;
    user_data[1].id_ = 1;

    for (int i = 0; i < desc_itf->bNumEndpoints; i++) {
        PRINTF("Endpoint %d %d %d\n", desc_ep->bEndpointAddress, tu_edpt_dir(desc_ep->bEndpointAddress),
               desc_ep->bDescriptorType);

        if (TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType) {
            if (tu_edpt_dir(desc_ep->bEndpointAddress) == TUSB_DIR_IN) {
                // skip if failed to open endpoint
                if (!open_input_endpoint(daddr, desc_ep, 0)) {
                    PRINTF("open_input_endpoint failed!\n");
                    return;
                }

                tusb_desc_endpoint_t second;
                second = *desc_ep;
                second.bEndpointAddress = 0x83;

                if (!open_input_endpoint(daddr, &second, 1)) {
                    PRINTF("open_input_endpoint failed!\n");
                    return;
                }
            }
        }

        p_desc = tu_desc_next(p_desc);
        desc_ep = (tusb_desc_endpoint_t const *)p_desc;
    }
}

void c_report_received(tuh_xfer_t *xfer) {
    // Note: not all fields in xfer are available for use (i.e filled by tinyusb stack) in callback to save sram
    // For instance, xfer->buffer is NULL. We have used user_data to store buffer when submitted callback
    auto obj = reinterpret_cast<Xbox360WirelessReceiverHandler::WirelessGamepadInstance *>(xfer->user_data);
    obj->handler_->report_received(xfer);
}
