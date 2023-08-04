#include "hid.hpp"

#include "quadrature_encoder.hpp"

#include "bsp/board.h"
#include "pico/stdlib.h"
#include "tusb.h"

struct MouseReport {
    struct {
        uint8_t button_left : 1;
        uint8_t button_right : 1;
        uint8_t button_middle : 1;
        uint8_t reserved : 5;
    };

    int8_t relx, rely, wheel;
};

class MouseHandler : public HidHandler {
  private:
    QuadratureEncoder h;
    QuadratureEncoder v;
    uint32_t last_update;

  public:
    void process_report(std::span<const uint8_t> report) override {

#if 0
        printf("Mouse:");
        for (uint8_t i : report) {
            printf(" %02x", i);
        }
        printf("\r\n");
#endif

        auto mouse_report =
            reinterpret_cast<const struct MouseReport *>(report.data());

        h.add_to_accumulator(mouse_report->relx);
        v.add_to_accumulator(mouse_report->rely);

#if 0
        gpio_put(4, mouse_report->button_left);
        gpio_put(6, mouse_report->button_right);
#else
        gpio_put(9, mouse_report->button_left);
        gpio_put(7, mouse_report->button_right);
#endif

        /*
        printf("%d%d%d %d %d\r\n", mouse_report->button_left,
                mouse_report->button_middle, mouse_report->button_right,
                mouse_report->relx, mouse_report->rely);
                */
    }

    void run() override {

        uint32_t now = timer_hw->timerawl;

        uint32_t time_diff = now - last_update;

        if (time_diff > 20) {
            last_update = now;
            auto h_state = h.update();
            auto v_state = v.update();

#if 0
            gpio_put(1, v_state.first);
            gpio_put(3, v_state.second);
            gpio_put(0, h_state.first);
            gpio_put(2, h_state.second);
#else
            gpio_put(12, v_state.first);
            gpio_put(15, v_state.second);
            gpio_put(10, h_state.first);
            gpio_put(14, h_state.second);
#endif
        }
        // printf("%d%d\r\n", v_state.first, v_state.second);
    }
};

static HidHandlerBuilder
    builder(0, 0, nullptr, [](tuh_hid_report_info_t *info) {
        if (info->usage == HID_USAGE_DESKTOP_MOUSE &&
            info->usage_page == HID_USAGE_PAGE_DESKTOP) {
            return std::make_unique<MouseHandler>();
        } else {
            return std::unique_ptr<MouseHandler>(nullptr);
        }
    });
