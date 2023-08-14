#pragma once

#define PIO0_BASE 0
typedef void pio_program_t;

typedef void *PIO;

bool pio_sm_is_tx_fifo_empty(PIO pio, uint sm);
void pio_sm_put(PIO pio, uint sm, uint32_t data);
void pio_sm_set_enabled(PIO pio, uint sm, bool enabled);
void sid_adc_stim_program_init(PIO pio, uint sm, uint offset, uint sense_pin,
                               uint drive_pin);
uint pio_add_program(PIO pio, const pio_program_t *program);

static inline char sid_adc_stim_program[10];
