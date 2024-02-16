
// stolen from https://github.com/quantus/xbox-one-controller-protocol
// https://github.com/felis/USB_Host_Shield_2.0/blob/master/XBOXONE

#include "bare_xbox_one.hpp"

struct __attribute__((packed)) XboxOneButtonData {
    uint8_t type;
    uint8_t const_0;
    uint16_t id;

    bool sync : 1;
    bool dummy1 : 1; // Always 0.
    bool start : 1;
    bool back : 1;

    bool a : 1;
    bool b : 1;
    bool x : 1;
    bool y : 1;

    bool dpad_up : 1;
    bool dpad_down : 1;
    bool dpad_left : 1;
    bool dpad_right : 1;

    bool bumper_left : 1;
    bool bumper_right : 1;
    bool stick_left_click : 1;
    bool stick_right_click : 1;

    uint16_t trigger_left;
    uint16_t trigger_right;

    int16_t stick_left_x;
    int16_t stick_left_y;
    int16_t stick_right_x;
    int16_t stick_right_y;
};

void c_report_received(tuh_xfer_t *xfer);

void configure_finished_received(tuh_xfer_t *xfer) {
    std::ignore = xfer;
    PRINTF("configure_finished_received %d\r\n", xfer->result);
}

void XboxOneHandler::report_received(tuh_xfer_t *xfer) {
    if (xfer->result == XFER_RESULT_SUCCESS) {
        static constexpr int16_t kAnalogThreshold{16000};
        static constexpr uint8_t kTypeButtonData{0x20};

        auto dat = reinterpret_cast<const XboxOneButtonData *>(buf_in.data());
        if (dat->type == kTypeButtonData) {
#if 0
            PRINTF("XboxOne: %d%d%d%d %d%d%d%d %d %d %d\r\n", dat->dpad_down, dat->dpad_left, dat->dpad_right,
                   dat->dpad_up, dat->x, dat->y, dat->b, dat->a, dat->stick_left_x, dat->stick_left_y, dat->back);
#endif
            GamepadReport aj;

            aj.left = dat->dpad_left || dat->stick_left_x < (-kAnalogThreshold);
            aj.down = dat->dpad_down || dat->stick_left_y < (-kAnalogThreshold);
            aj.right = dat->dpad_right || dat->stick_left_x > (+kAnalogThreshold);
            aj.up = dat->dpad_up || dat->stick_left_y > (+kAnalogThreshold);

            aj.fire = dat->x || dat->b;
            aj.sec_fire = dat->a;
            aj.auto_fire = dat->y;

            aj.joystick_swap = dat->back;

            if (target_) {
                target_->process_gamepad_report(aj);
            }
        }
    }

    // continue to submit transfer, with updated buffer
    // other field remain the same
    xfer->buflen = 64;
    xfer->buffer = buf_in.data();

    tuh_edpt_xfer(xfer);
}

void XboxOneHandler::open_vendor_interface(uint8_t daddr, tusb_desc_interface_t const *desc_itf, uint16_t max_len) {
    // len = interface + hid + n*endpoints
    uint16_t const drv_len = (uint16_t)(sizeof(tusb_desc_interface_t) + sizeof(tusb_hid_descriptor_hid_t) +
                                        desc_itf->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

    // corrupted descriptor
    if (max_len < drv_len)
        return;

    uint8_t const *p_desc = (uint8_t const *)desc_itf;

    // Endpoint descriptor
    p_desc = tu_desc_next(p_desc);
    tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;

    for (int i = 0; i < desc_itf->bNumEndpoints; i++) {
        if (TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType) {

            if (tu_edpt_dir(desc_ep->bEndpointAddress) == TUSB_DIR_IN) {
                // skip if failed to open endpoint
                if (!tuh_edpt_open(daddr, desc_ep))
                    return;

                tuh_xfer_t xfer = {.daddr = daddr,
                                   .ep_addr = desc_ep->bEndpointAddress,
                                   .reserved2 = 0,
                                   .result = XFER_RESULT_SUCCESS,
                                   .actual_len = 0,
                                   .buflen = buf_in.size(),
                                   .buffer = buf_in.data(),
                                   .complete_cb = c_report_received,
                                   .user_data = reinterpret_cast<uintptr_t>(this)};

                PRINTF("Listen to [dev %u: ep %02x]\r\n", daddr, desc_ep->bEndpointAddress);

                // submit transfer for this EP
                bool result = tuh_edpt_xfer(&xfer);
                std::ignore = result; // TODO proper error handling

                PRINTF("in %d\r\n", result);
            }
            if (tu_edpt_dir(desc_ep->bEndpointAddress) == TUSB_DIR_OUT) {
                // skip if failed to open endpoint
                if (!tuh_edpt_open(daddr, desc_ep))
                    return;

                buf_out[0] = 0x05;
                buf_out[1] = 0x20;
                buf_out[2] = 0x00;
                buf_out[3] = 0x01;
                buf_out[4] = 0x00;

                tuh_xfer_t xfer = {.daddr = daddr,
                                   .ep_addr = desc_ep->bEndpointAddress,
                                   .reserved2 = 0,
                                   .result = XFER_RESULT_SUCCESS,
                                   .actual_len = 0,
                                   .buflen = 5,
                                   .buffer = buf_out.data(),
                                   .complete_cb = configure_finished_received,
                                   .user_data = reinterpret_cast<uintptr_t>(this)};

                PRINTF("Send to [dev %u: ep %02x]\r\n", daddr, desc_ep->bEndpointAddress);

                // submit transfer for this EP
                bool result = tuh_edpt_xfer(&xfer);
                std::ignore = result; // TODO proper error handling

                PRINTF("out %d\r\n", result);
            }
        }

        p_desc = tu_desc_next(p_desc);
        desc_ep = (tusb_desc_endpoint_t const *)p_desc;
    }
}

void c_report_received(tuh_xfer_t *xfer) {
    // Note: not all field in xfer is available for use (i.e filled by tinyusb stack) in callback to save sram
    // For instance, xfer->buffer is NULL. We have used user_data to store buffer when submitted callback
    auto obj = reinterpret_cast<XboxOneHandler *>(xfer->user_data);
    obj->report_received(xfer);
}
