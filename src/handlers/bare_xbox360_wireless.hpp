#pragma once

#include "bare_api.hpp"
#include "processors/interfaces.hpp"
#include "tusb.h"
#include "utility.h"

// PID and VID of the different devices
// got this list from here https://github.com/felis/USB_Host_Shield_2.0/blob/master/XBOXRECV.h
#define XBOX_VID 0x045E    // Microsoft Corporation
#define MADCATZ_VID 0x1BAD // For unofficial Mad Catz receivers
#define JOYTECH_VID 0x162E // For unofficial Joytech controllers

#define XBOX_WIRELESS_RECEIVER_PID_1 0x0719           // Microsoft Wireless Gaming Receiver
#define XBOX_WIRELESS_RECEIVER_PID_2 0x02A9           // Microsoft Wireless Gaming Receiver
#define XBOX_WIRELESS_RECEIVER_THIRD_PARTY_PID 0x0291 // Third party Wireless Gaming Receiver

static inline bool check_xbox_360_wireless_receiver_vid_pid(uint16_t vid, uint16_t pid) {
    return !((vid != XBOX_VID && vid != MADCATZ_VID && vid != JOYTECH_VID) ||
             (pid != XBOX_WIRELESS_RECEIVER_PID_1 && pid != XBOX_WIRELESS_RECEIVER_PID_2 &&
              pid != XBOX_WIRELESS_RECEIVER_THIRD_PARTY_PID));
}

/**
 * @brief Handles the USB vendor class interface of Xbox 360 Wireless Receivers
 *
 * Xbox 360 Wireless Receivers are no standard HID interfaces.
 * They don't even have valid config descriptors as not all Endpoints are advertised
 */
class Xbox360WirelessReceiverHandler : public ReportSourceInterface {
  private:
    /// @brief Proxy class which can provide gamepad reports
    /// This is required due to the nature of \ref Xbox360WirelessReceiverHandler
    /// having support for two game pads. The pipeline shall perceive them as
    /// two separate entities
    class ReportProxy : public ReportSourceInterface {
      public:
        /// @brief data sink to feed reports to
        std::shared_ptr<ReportHubInterface> target_;

        ReportProxy() {
            PRINTF("ReportProxy +\n");
        }
        virtual ~ReportProxy() {
            PRINTF("ReportProxy -\n");
        }

        void set_target(std::shared_ptr<ReportHubInterface> target) override {
            target_ = target;
        }

        ReportType expected_report() override {
            return kGamePad;
        }

        void run() override {};
    };

  public:
    /// @brief All data required to managed one USB connection for one Xbox 360 Wireless Gamepad
    class WirelessGamepadInstance {
      public:
        /// @brief 0 for first gamepad. 1 for second
        uint32_t id_;

        /// @brief Pointer to father class.
        /// This object is used as user data for tinyusb
        Xbox360WirelessReceiverHandler *handler_;
        /// @brief USB Interrupt Endpoint buffer for incoming data
        std::array<uint8_t, 64> buf_in_;
        /// @brief USB Interrupt Endpoint buffer for outgoing data
        std::array<uint8_t, 64> buf_out_;

        /// @brief TinyUSB transfer handler for incoming data
        tuh_xfer_t xfer_in_;
        /// @brief TinyUSB transfer handler for outgoing data
        tuh_xfer_t xfer_out_;

        /// @brief Access to report proxy to gamepad reports to
        /// If null, this gamepad is currently not connected
        std::shared_ptr<ReportProxy> report_proxy_;
    };

  private:
    /// @brief
    /// @param daddr     TinyUSB device identifier
    /// @param desc_ep   Pointer to endpoint description in config descriptor
    /// @param index     0 for first controller, 1 for second
    /// @return          True if successful
    bool open_input_endpoint(uint8_t daddr, tusb_desc_endpoint_t const *desc_ep, int index);

  protected:
    /// @brief Buffer for input endpoint
    /// Stores input data from controller
    std::array<WirelessGamepadInstance, 4> user_data;

  public:
    Xbox360WirelessReceiverHandler() {
        PRINTF("Xbox360WirelessReceiverHandler +\n");
    }
    virtual ~Xbox360WirelessReceiverHandler() {
        PRINTF("Xbox360WirelessReceiverHandler -\n");
    }

    void set_target(std::shared_ptr<ReportHubInterface>) override {
        PRINTF("Not expected. Must never be called!\n");
        abort();
    }

    ReportType expected_report() override {
        PRINTF("Not expected. Must never be called!\n");
        abort();
        return kGamePad;
    }

    /**
     * @brief Callback function for received input data
     * Keep in mind that the buffer is not part of the parameter.
     *
     * @param xfer  TinyUSB transfer
     */
    void report_received(tuh_xfer_t *xfer);

    /**
     * @brief Initializes Xbox 360 Wireless receiver
     *
     * @param daddr         TinyUSB device identifier
     * @param desc_itf      Device descriptor to process
     * @param max_len       Length of data referenced by desc_itf
     */
    void open_vendor_interface(uint8_t daddr, tusb_desc_interface_t const *desc_itf, uint16_t max_len);

    void run() override {};
};
