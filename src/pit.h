//
// Created by T480 on 07.09.2025.
//

#ifndef PIT_H
#define PIT_H

#include <stdint.h>

/* Initialize PIT to 'hz' ticks per second (e.g., 1000). */
void pit_init(uint32_t hz);

/* Monotonic tick counter since boot (wraps after ~584 years at 1 kHz). */
uint64_t pit_ticks(void);

/* Cheap timing helpers derived from the configured Hz. */
uint32_t pit_hz(void);
uint64_t time_ms(void);  /* milliseconds since boot */
uint64_t time_us(void);  /* microseconds since boot (coarse) */

#endif //PIT_H
