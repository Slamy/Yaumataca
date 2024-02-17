
// stolen from https://github.com/quantus/xbox-one-controller-protocol
// https://github.com/felis/USB_Host_Shield_2.0/blob/master/XBOXONE

#include "bare_api.hpp"
#include "processors/interfaces.hpp"
#include "tusb.h"
#include "utility.h"

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

    void run() override{};
};
