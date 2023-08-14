#pragma once

#include "bsp/board.h"

static inline uint32_t board_micros(void) { return timer_hw->timerawl; }
