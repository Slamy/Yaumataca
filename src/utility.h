#pragma once

#include <limits>

#ifdef DEBUG_PRINT
#define PRINTF(...) printf(__VA_ARGS__)
#else
// Remove PRINTF from all source code
#define PRINTF(...)
#endif

#include "bsp/board_api.h"

static inline uint32_t board_micros(void) {
    return timer_hw->timerawl;
}

/// Address in flash where the current mouse mode is stored
/// via Flash EEPROM emulation
/// Must be dividable by 4096 which is the Flash erase sector size
static constexpr uint32_t kFlashMouseModeOffset{0x40000};

/// Address in flash where the C1351 emulation calibration data is stored
/// Must be dividable by 4096 which is the Flash erase sector size
static constexpr uint32_t kFlashCalibrationDataOffset{0x41000};

static inline int8_t saturating_cast(int32_t val) {
    if (val > std::numeric_limits<int8_t>::max())
        return std::numeric_limits<int8_t>::max();
    else if (val < std::numeric_limits<int8_t>::min())
        return std::numeric_limits<int8_t>::min();
    else
        return static_cast<int8_t>(val);
}