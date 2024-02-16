#include "mouse_c1351.hpp"
#include "hardware/flash.h"
#include "utility.h"

static const uint8_t *flash_target_contents = (const uint8_t *)(XIP_BASE + kFlashCalibrationDataOffset);

std::array<struct C1351CalibrationData, 2> C1351Converter::calibration_;
static std::array<uint8_t, FLASH_PAGE_SIZE> page_buffer_;

void C1351Converter::save_calibration_data() {

    flash_range_erase(kFlashCalibrationDataOffset, FLASH_SECTOR_SIZE);

    memcpy(page_buffer_.data(), calibration_.data(), sizeof(calibration_));
    strcpy((char *)&page_buffer_.at(sizeof(calibration_)), "VALID");

    flash_range_program(kFlashCalibrationDataOffset, page_buffer_.data(), page_buffer_.size());
    PRINTF("Calibration data stored\n");
}

void C1351Converter::load_calibration_data() {
    const char *validity_string = (const char *)&flash_target_contents[sizeof(calibration_)];
    if (!strcmp("VALID", validity_string)) {
        PRINTF("C1351 calibration previously stored. Use it!\n");
        memcpy(calibration_.data(), flash_target_contents, sizeof(calibration_));
    } else {
        PRINTF("C1351 calibration data not saved before...\n");
    }

    for (auto &calib : calibration_) {
        PRINTF("C1351 calibration %ld %ld %ld %ld\n", calib.pot_x_64_, calib.pot_x_191_, calib.pot_y_64_,
               calib.pot_y_191_);

        // required in case PRINTF is deactivated
        std::ignore = calib;
    }
}
