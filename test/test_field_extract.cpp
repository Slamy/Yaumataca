
#include <cstdio>
#include <memory>

#include "field_extractor.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <queue>

TEST(FieldExtractor, Extract) {
    {
        // Example from rapoo mouse
        // Left mouse button, going left down and some wheel as well
        // 01 01 f6 ff 08 00 02
        // X is at offset 16
        FieldExtractor x, y, wheel, buttons, report_id;
        buttons.configure(8, 5, false);
        x.configure(16, 16, true);
        y.configure(32, 16, true);
        wheel.configure(48, 8, true);
        report_id.configure(0, 8, false);
        std::vector<uint8_t> data({0x01, 0x01, 0xf6, 0xff, 0x08, 0x00, 0x02});
        EXPECT_EQ(x.extract(data.data(), data.size()), -10);
        EXPECT_EQ(y.extract(data.data(), data.size()), 8);
        EXPECT_EQ(wheel.extract(data.data(), data.size()), 2);
        EXPECT_EQ(buttons.extract(data.data(), data.size()), 1);
        EXPECT_EQ(report_id.extract(data.data(), data.size()), 1);
    }

    {
        // Another Example from rapoo mouse
        // 01 01 ff fe ff ff fe
        // X is at offset 16
        FieldExtractor x, y, wheel, buttons, report_id;
        buttons.configure(8, 5, false);
        x.configure(16, 16, true);
        y.configure(32, 16, true);
        wheel.configure(48, 8, true);
        report_id.configure(0, 8, false);
        std::vector<uint8_t> data({0x01, 0x03, 0xff, 0xfe, 0xff, 0xff, 0xfe});
        EXPECT_EQ(x.extract(data.data(), data.size()), -257);
        EXPECT_EQ(y.extract(data.data(), data.size()), -1);
        EXPECT_EQ(wheel.extract(data.data(), data.size()), -2);
        EXPECT_EQ(buttons.extract(data.data(), data.size()), 3);
        EXPECT_EQ(report_id.extract(data.data(), data.size()), 1);
    }

    {
        // Non byte aligned, 4 bits
        FieldExtractor dut;
        dut.configure(0, 4, false);
        std::vector<uint8_t> data({0x12, 0x34, 0x56});
        auto result = dut.extract(data.data(), data.size());
        EXPECT_EQ(result, 2);
    }

    {
        // Non byte aligned, 5 bits
        FieldExtractor dut;
        dut.configure(0, 5, false);
        std::vector<uint8_t> data({0x12, 0x34, 0x56});
        auto result = dut.extract(data.data(), data.size());
        EXPECT_EQ(result, 0x12);
    }

    {
        // Non byte aligned, Now with sign extensions and 5 bits
        FieldExtractor dut;
        dut.configure(0, 5, true);
        std::vector<uint8_t> data({0x12, 0x34, 0x56});
        auto result = dut.extract(data.data(), data.size());
        EXPECT_EQ(result, -14);
    }
}