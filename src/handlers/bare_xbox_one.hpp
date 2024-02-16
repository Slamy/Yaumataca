
// stolen from https://github.com/quantus/xbox-one-controller-protocol
// https://github.com/felis/USB_Host_Shield_2.0/blob/master/XBOXONE

#include "bare_api.hpp"
#include "processors/interfaces.hpp"
#include "tusb.h"
#include "utility.h"

class XboxOneHandler : public ReportSourceInterface {
  protected:
    /// @brief data sinkt to feed reports to
    std::shared_ptr<ReportHubInterface> target_;

    std::array<uint8_t, 64> buf_in;
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

    void report_received(tuh_xfer_t *xfer);
    void open_vendor_interface(uint8_t daddr, tusb_desc_interface_t const *desc_itf, uint16_t max_len);
    void run() override{};
};
