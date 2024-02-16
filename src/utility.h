#pragma once

#ifdef DEBUG_PRINT
#define PRINTF(...) printf(__VA_ARGS__)
#else
// Remove PRINTF from all source code
#define PRINTF(...)
#endif

#include "bsp/board.h"

static inline uint32_t board_micros(void) {
    return timer_hw->timerawl;
}

/// Address in flash where the current mouse mode is stored
/// via Flash EEPROM emulation
/// Must be dividable by 4096 which is the Flash erase sector size
static constexpr uint32_t kFlashMouseModeOffset{0x40000};