
#include <cstdio>
#include <memory>

#include "processors/pipeline.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <queue>

#include "fff.h"
DEFINE_FFF_GLOBALS;

uint32_t global_time_us{0};

uint32_t board_millis() { return global_time_us / 1000; }

uint32_t board_micros() { return global_time_us; }

// FAKE_VALUE_FUNC(uint32_t, board_micros);
FAKE_VOID_FUNC(board_led_write, bool);

FAKE_VALUE_FUNC(bool, pio_sm_is_tx_fifo_empty, PIO, uint);
FAKE_VOID_FUNC(pio_sm_put, PIO, uint, uint32_t);

FAKE_VOID_FUNC(pio_sm_set_enabled, PIO, uint, bool);
FAKE_VOID_FUNC(sid_adc_stim_program_init, PIO, uint, uint, uint, uint);
FAKE_VALUE_FUNC(uint, pio_add_program, PIO, const pio_program_t *);

using testing::_;

class MockControllerPort : public ControllerPortInterface {
  public:
    MOCK_METHOD(void, set_port_state, (ControllerPortState & state));
    MOCK_METHOD(uint, get_pot_x_drain_gpio, ());
    MOCK_METHOD(uint, get_pot_y_drain_gpio, ());
    MOCK_METHOD(uint, get_pot_y_sense_gpio, ());
    MOCK_METHOD(void, configure_gpios, ());

    const char *get_name() override { return ""; }
};

class MockHidHandler : public HidHandlerInterface {
  private:
    ReportType type_;

  public:
    MockHidHandler(ReportType t) : type_(t) {}
    std::shared_ptr<ReportHubInterface> target_;

    void setup_reception(int8_t dev_addr, uint8_t instance) override{};

    void process_report(std::span<const uint8_t> report) override{};

    ReportType expected_report() { return type_; }
    void set_target(std::shared_ptr<ReportHubInterface> target) override {
        target_ = target;
    };

    void run() override{};
};
PIO C1351Converter::pio_{nullptr};
uint C1351Converter::offset_{0};

ControllerPortState cps_from_text(const char *text) {
    ControllerPortState result;

    result.up = text[0] == '1';
    result.down = text[1] == '1';
    result.left = text[2] == '1';
    result.right = text[3] == '1';

    result.fire1 = text[5] == '1';
    result.fire2 = text[6] == '1';
    result.fire3 = text[7] == '1';

    return result;
}

void cps_to_text(ControllerPortState state) {
    printf("cps_from_text(\"%d%d%d%d %d%d%d\");\n", state.up, state.down,
           state.left, state.right, state.fire1, state.fire2, state.fire3);
}

void expect_str(ControllerPortState state) {
    printf("expect (\"%d%d%d%d %d%d%d\");\n", state.up, state.down, state.left,
           state.right, state.fire1, state.fire2, state.fire3);
}

TEST(Pipeline, SingleJoystickAndAmigaMouse) {

    std::shared_ptr<MockControllerPort> port_joy =
        std::make_shared<MockControllerPort>();
    std::shared_ptr<MockControllerPort> port_mouse =
        std::make_shared<MockControllerPort>();

    EXPECT_CALL(*port_joy, configure_gpios).Times(testing::AtLeast(1));
    EXPECT_CALL(*port_mouse, configure_gpios).Times(testing::AtLeast(1));

    ON_CALL(*port_mouse, set_port_state(_))
        .WillByDefault([](ControllerPortState state) { cps_to_text(state); });

    auto pipeline = std::make_unique<Pipeline>(port_joy, port_mouse);

    std::shared_ptr<MockHidHandler> mock_joy =
        std::make_shared<MockHidHandler>(ReportType::kGamePad);
    std::shared_ptr<MockHidHandler> mock_mouse =
        std::make_shared<MockHidHandler>(ReportType::kMouse);

    EXPECT_FALSE(mock_joy->target_);
    EXPECT_FALSE(mock_mouse->target_);

    pipeline->integrate_handler(mock_joy);
    pipeline->integrate_handler(mock_mouse);

    EXPECT_TRUE(mock_joy->target_);
    EXPECT_TRUE(mock_mouse->target_);

    ControllerPortState nothing_pressed;

    // Press the fire button
    {
        ControllerPortState expected_state;
        expected_state.fire1 = true;

        EXPECT_CALL(*port_joy, set_port_state(expected_state));

        GamepadReport report;
        report.fire = 1;

        pipeline->run();
        mock_joy->target_->process_gamepad_report(report);
        pipeline->run();
    }

    // Release the fire button
    {
        ControllerPortState expected_state;
        EXPECT_CALL(*port_joy, set_port_state(expected_state));

        GamepadReport report;

        pipeline->run();
        mock_joy->target_->process_gamepad_report(report);
        pipeline->run();
    }

    global_time_us += 10 * 1000;
    pipeline->run();
    global_time_us += 10 * 1000;
    pipeline->run();
    global_time_us += 10 * 1000;
    pipeline->run();

    // Move the mouse horizontally
    printf("Move the mouse horizontally...\n");
    {
        MouseReport report;
        report.relx = 9;
        mock_mouse->target_->process_mouse_report(report);

        ControllerPortState expected_state;

        std::queue<ControllerPortState> states;

        states.push(cps_from_text("0100 000"));
        states.push(cps_from_text("0101 000"));
        states.push(cps_from_text("0001 000"));
        states.push(cps_from_text("0000 000"));
        states.push(cps_from_text("0100 000"));
        states.push(cps_from_text("0101 000"));
        states.push(cps_from_text("0001 000"));
        states.push(cps_from_text("0000 000"));
        states.push(cps_from_text("0100 000"));
        {
            testing::InSequence s;

            while (!states.empty()) {
                expected_state = states.front();
                expect_str(expected_state);
                EXPECT_CALL(*port_mouse, set_port_state(expected_state));
                states.pop();
            }
        }

        for (int i = 0; i < 30; i++) {
            global_time_us += AmigaMouse::kUpdatePeriod;
            pipeline->run();
        }
    }

    // Move the mouse vertically
    printf("Move the mouse vertically...\n");
    {
        MouseReport report;
        report.rely = 5;
        mock_mouse->target_->process_mouse_report(report);

        ControllerPortState expected_state;

        std::queue<ControllerPortState> states;

        states.push(cps_from_text("1100 000"));
        states.push(cps_from_text("1110 000"));
        states.push(cps_from_text("0110 000"));
        states.push(cps_from_text("0100 000"));
        states.push(cps_from_text("1100 000"));

        {
            testing::InSequence s;

            while (!states.empty()) {
                expected_state = states.front();
                expect_str(expected_state);
                EXPECT_CALL(*port_mouse, set_port_state(expected_state));
                states.pop();
            }
        }

        for (int i = 0; i < 30; i++) {
            global_time_us += AmigaMouse::kUpdatePeriod;
            pipeline->run();
        }
    }

    printf("Move the mouse vertically back...\n");
    {
        MouseReport report;
        report.rely = -5;
        mock_mouse->target_->process_mouse_report(report);

        ControllerPortState expected_state;

        std::queue<ControllerPortState> states;
        states.push(cps_from_text("0100 000"));
        states.push(cps_from_text("0110 000"));
        states.push(cps_from_text("1110 000"));
        states.push(cps_from_text("1100 000"));
        states.push(cps_from_text("0100 000"));

        {
            testing::InSequence s;

            while (!states.empty()) {
                expected_state = states.front();
                expect_str(expected_state);
                EXPECT_CALL(*port_mouse, set_port_state(expected_state));
                states.pop();
            }
        }

        for (int i = 0; i < 30; i++) {
            global_time_us += AmigaMouse::kUpdatePeriod;
            pipeline->run();
        }
    }

    // Press the second fire button
    {
        ControllerPortState expected_state;
        expected_state.fire2 = true;

        EXPECT_CALL(*port_joy, set_port_state(expected_state));

        GamepadReport report;
        report.sec_fire = 1;

        pipeline->run();
        mock_joy->target_->process_gamepad_report(report);
        pipeline->run();
    }

    // Release the second fire button
    {
        ControllerPortState expected_state;
        EXPECT_CALL(*port_joy, set_port_state(expected_state));

        GamepadReport report;

        pipeline->run();
        mock_joy->target_->process_gamepad_report(report);
        pipeline->run();
    }

    // Swap the joysticks

    {
        GamepadReport report;
        report.joystick_swap = 1;
        pipeline->run();
        mock_joy->target_->process_gamepad_report(report);

        for (int i = 0; i < 20; i++) {
            global_time_us += 1000 * 100; // 4 seconds should be enough
            pipeline->run();
        }
    }

    // Press the fire button again and expect it from the mouse
    {
        ControllerPortState expected_state;
        expected_state.fire1 = true;

        EXPECT_CALL(*port_mouse, set_port_state(expected_state));

        GamepadReport report;
        report.fire = 1;

        pipeline->run();
        mock_joy->target_->process_gamepad_report(report);
        pipeline->run();
    }

    // Release the second fire button and expect it from the mouse
    {
        ControllerPortState expected_state;
        EXPECT_CALL(*port_mouse, set_port_state(expected_state));

        GamepadReport report;

        pipeline->run();
        mock_joy->target_->process_gamepad_report(report);
        pipeline->run();
    }
}

TEST(Pipeline, DualAmigaMouse) {

    std::shared_ptr<MockControllerPort> port_joy =
        std::make_shared<MockControllerPort>();
    std::shared_ptr<MockControllerPort> port_mouse =
        std::make_shared<MockControllerPort>();

    EXPECT_CALL(*port_joy, configure_gpios).Times(testing::AtLeast(1));
    EXPECT_CALL(*port_mouse, configure_gpios).Times(testing::AtLeast(1));

    ON_CALL(*port_mouse, set_port_state(_))
        .WillByDefault([](ControllerPortState state) { cps_to_text(state); });

    auto pipeline = std::make_unique<Pipeline>(port_joy, port_mouse);

    std::shared_ptr<MockHidHandler> mock_mouse2 =
        std::make_shared<MockHidHandler>(ReportType::kMouse);
    std::shared_ptr<MockHidHandler> mock_mouse1 =
        std::make_shared<MockHidHandler>(ReportType::kMouse);

    EXPECT_FALSE(mock_mouse2->target_);
    EXPECT_FALSE(mock_mouse1->target_);

    pipeline->integrate_handler(mock_mouse1);
    pipeline->integrate_handler(mock_mouse2);

    EXPECT_TRUE(mock_mouse1->target_);
    EXPECT_TRUE(mock_mouse2->target_);

    // Move the mouse horizontally

    MouseReport report1;
    report1.relx = 9;
    mock_mouse1->target_->process_mouse_report(report1);

    MouseReport report2;
    report2.rely = 9;
    mock_mouse2->target_->process_mouse_report(report2);

    ControllerPortState expected_state;

    {
        std::queue<ControllerPortState> states;

        states.push(cps_from_text("0100 000"));
        states.push(cps_from_text("0101 000"));
        states.push(cps_from_text("0001 000"));
        states.push(cps_from_text("0000 000"));
        states.push(cps_from_text("0100 000"));
        states.push(cps_from_text("0101 000"));
        states.push(cps_from_text("0001 000"));
        states.push(cps_from_text("0000 000"));
        states.push(cps_from_text("0100 000"));

        {
            testing::InSequence s;

            while (!states.empty()) {
                expected_state = states.front();
                expect_str(expected_state);
                EXPECT_CALL(*port_mouse, set_port_state(expected_state));
                states.pop();
            }
        }
    }

    {
        std::queue<ControllerPortState> states;

        states.push(cps_from_text("1000 000"));
        states.push(cps_from_text("1010 000"));
        states.push(cps_from_text("0010 000"));
        states.push(cps_from_text("0000 000"));
        states.push(cps_from_text("1000 000"));
        states.push(cps_from_text("1010 000"));
        states.push(cps_from_text("0010 000"));
        states.push(cps_from_text("0000 000"));
        states.push(cps_from_text("1000 000"));
        {
            testing::InSequence s;

            while (!states.empty()) {
                expected_state = states.front();
                expect_str(expected_state);
                EXPECT_CALL(*port_joy, set_port_state(expected_state));
                states.pop();
            }
        }
    }

    for (int i = 0; i < 30; i++) {
        global_time_us += AmigaMouse::kUpdatePeriod;
        pipeline->run();
    }
}

TEST(Pipeline, DualJoystick) {

    std::shared_ptr<MockControllerPort> port_joy =
        std::make_shared<MockControllerPort>();
    std::shared_ptr<MockControllerPort> port_mouse =
        std::make_shared<MockControllerPort>();

    EXPECT_CALL(*port_joy, configure_gpios);
    EXPECT_CALL(*port_mouse, configure_gpios);

    auto pipeline = std::make_unique<Pipeline>(port_joy, port_mouse);

    std::shared_ptr<MockHidHandler> mock_joy1 =
        std::make_shared<MockHidHandler>(ReportType::kGamePad);
    std::shared_ptr<MockHidHandler> mock_joy2 =
        std::make_shared<MockHidHandler>(ReportType::kGamePad);

    pipeline->integrate_handler(mock_joy1);
    pipeline->integrate_handler(mock_joy2);

    {
        ControllerPortState expected_state;
        expected_state.fire1 = 1;
        EXPECT_CALL(*port_joy, set_port_state(expected_state));

        GamepadReport report1;
        report1.fire = 5;
        mock_joy1->target_->process_gamepad_report(report1);
    }
    {
        ControllerPortState expected_state;
        expected_state.up = 1;
        EXPECT_CALL(*port_mouse, set_port_state(expected_state));

        GamepadReport report2;
        report2.up = 5;
        mock_joy2->target_->process_gamepad_report(report2);
    }
    pipeline->run();
}