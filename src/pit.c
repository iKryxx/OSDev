#include "pit.h"
#include "io.h"
#include "irq.h"   // irq_register(irq, handler, ctx)
#include <stdint.h>

#include "idt.h"

#define PIT_CH0  0x40
#define PIT_CMD  0x43
#define PIT_BASE 1193182u   /* Hz */

static volatile uint64_t g_ticks = 0;
static uint32_t g_hz = 0;

/* IRQ0 handler: just count ticks. Keep it tiny. */
static void pit_irq(void* ctx, const regs_t* r) {
    (void)ctx; (void)r;
    g_ticks++;
}

uint64_t pit_ticks(void) { return g_ticks; }
uint32_t pit_hz(void)    { return g_hz;   }

void pit_init(uint32_t hz) {
    if (hz < 19)  hz = 19;        /* 16-bit divisor limit */
    if (hz > 50000) hz = 50000;   /* keep latency reasonable */

    uint32_t divisor = PIT_BASE / hz;
    if (divisor == 0) divisor = 1;

    /* Command: ch0, access lobyte/hibyte, mode 2 (rate gen), binary */
    outb(PIT_CMD, 0x34);
    outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF));

    g_hz = hz;
    irq_register(0, pit_irq, 0);  /* IRQ0 */
}

uint64_t time_ms(void) {
    /* ms = ticks * 1000 / hz */
    uint64_t t = pit_ticks();
    return (t * 1000ull) / (g_hz ? g_hz : 1);
}

uint64_t time_us(void) {
    /* coarse us = ticks * 1e6 / hz  (integer math) */
    uint64_t t = pit_ticks();
    return (t * 1000000ull) / (g_hz ? g_hz : 1);
}
