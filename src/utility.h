#pragma once

#ifdef DEBUG_PRINT
//#define PRINTF(...) printf(__VA_ARGS__)
#else
// Remove PRINTF from all source code
#define PRINTF(...)
#endif

#include "bsp/board.h"

static inline uint32_t board_micros(void) { return timer_hw->timerawl; }
