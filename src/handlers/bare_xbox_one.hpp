
// stolen from https://github.com/quantus/xbox-one-controller-protocol
// https://github.com/felis/USB_Host_Shield_2.0/blob/master/XBOXONE

#include "bare_api.hpp"
#include "processors/interfaces.hpp"
#include "tusb.h"
#include "utility.h"

// PID and VID of the different versions of the controller - see:
// https://github.com/torvalds/linux/blob/master/drivers/input/joystick/xpad.c

// Official controllers
#define XBOX_VID1 0x045E      // Microsoft Corporation
#define XBOX_ONE_PID1 0x02D1  // Microsoft X-Box One pad
#define XBOX_ONE_PID2 0x02DD  // Microsoft X-Box One pad (Firmware 2015)
#define XBOX_ONE_PID3 0x02E3  // Microsoft X-Box One Elite pad
#define XBOX_ONE_PID4 0x02EA  // Microsoft X-Box One S pad
#define XBOX_ONE_PID13 0x0B0A // Microsoft X-Box One Adaptive Controller
#define XBOX_ONE_PID14 0x0B12 // Microsoft X-Box Core Controller

// Unofficial controllers
#define XBOX_VID2 0x0738 // Mad Catz
#define XBOX_VID3 0x0E6F // Afterglow
#define XBOX_VID4 0x0F0D // HORIPAD ONE
#define XBOX_VID5 0x1532 // Razer
#define XBOX_VID6 0x24C6 // PowerA

#define XBOX_ONE_PID5 0x4A01  // Mad Catz FightStick TE 2 - might have different mapping for triggers?
#define XBOX_ONE_PID6 0x0139  // Afterglow Prismatic Wired Controller
#define XBOX_ONE_PID7 0x0146  // Rock Candy Wired Controller for Xbox One
#define XBOX_ONE_PID8 0x0067  // HORIPAD ONE
#define XBOX_ONE_PID9 0x0A03  // Razer Wildcat
#define XBOX_ONE_PID10 0x541A // PowerA Xbox One Mini Wired Controller
#define XBOX_ONE_PID11 0x542A // Xbox ONE spectra
#define XBOX_ONE_PID12 0x543A // PowerA Xbox One wired controller

/**
 * @brief Handles the USB vendor class interface of Xbox One Controllers
 *
 * Xbox One Controllers are no standard HID interfaces.
 */
class XboxOneHandler : public ReportSourceInterface {
  protected:
    /// @brief data sinkt to feed reports to
    std::shared_ptr<ReportHubInterface> target_;

    /// @brief Buffer for input endpoint
    /// Stores input data from controller
    std::array<uint8_t, 64> buf_in;

    /// @brief Buffer for output endpoint
    /// Used to initialize the Controller to actually transmit data
    std::array<uint8_t, 64> buf_out;

  public:
    XboxOneHandler() {
        PRINTF("XboxOneHandler +\n");
    }
    virtual ~XboxOneHandler() {
        PRINTF("XboxOneHandler -\n");
    }

    void set_target(std::shared_ptr<ReportHubInterface> target) override {
        target_ = target;
    }

    ReportType expected_report() override {
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
     * @brief Initializes Xbox Controller
     *
     * Opens in and out endpoints and sends init command to activate input data transmission.
     *
     * @param daddr         TinyUSB device identifier
     * @param desc_itf      Device descriptor to process
     * @param max_len       Length of data referenced by desc_itf
     */
    void open_vendor_interface(uint8_t daddr, tusb_desc_interface_t const *desc_itf, uint16_t max_len);

    void run() override {};
};
