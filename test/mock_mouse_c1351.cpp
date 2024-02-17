#include "hardware/flash.h"
#include "mouse_c1351.hpp"
#include "utility.h"

std::array<struct C1351CalibrationData, 2> C1351Converter::calibration_;

void C1351Converter::save_calibration_data() {
}

void C1351Converter::load_calibration_data() {
}
