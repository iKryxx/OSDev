//
// Created by T480 on 07.09.2025.
//

#include <stdint.h>
#include <stddef.h>

#include "idt.h"
#include "io.h"
#include "pic.h"

static volatile uint16_t* const VGA = (uint16_t*)0xB8000;
static const int VGA_W = 80;
static const int VGA_H = 25;

static void vga_clear(uint8_t color) {
    uint16_t fill = ((uint16_t)color << 8) | ' ';
    for (int i = 0; i < VGA_W * VGA_H; ++i) VGA[i] = fill;
}

static void vga_puts_at(const char* s, int row, int col, uint8_t color) {
    int idx = row * VGA_W + col;
    while (*s && idx < VGA_W * VGA_H) {
        VGA[idx++] = ((uint16_t)color << 8) | (uint8_t)(*s++);
    }
}

extern void irq_install(int irq, void (*fn)(const regs_t*));
extern void isr_common_handler(const regs_t*); // just to satisfy prototype if needed
extern void irq_common_handler(const regs_t*); // optional


static volatile uint64_t g_ticks = 0;
static void timer_handler(const regs_t* r) {
    (void)r;
    ++g_ticks;
    /* simple, non-blocking: every ~18 ticks update a char */
    if ((g_ticks % 18) == 0) {
        static const char spin[4] = {'|','/','-','\\'};
        VGA[24 * 80 + 0] = ((uint16_t)0x0A << 8) | spin[(g_ticks/18) & 3];
    }
}

void kernel_main(void) {
    /* light-grey on black = 0x07 */
    vga_clear(0x07);
    vga_puts_at("Booting Up OS...", 0, 0, 0x0F);
    vga_puts_at("Transition from GRUB to long mode complete", 1, 0, 0x0A);

    /* --- NEW: set up IDT for exceptions --- */
    idt_init();
    vga_puts_at("Initializing IDT complete", 2, 0, 0x0A);

    /* remap PIC -> vectors 0x20-0x2F, install timer handler, enable ints */
    pic_remap(0x20, 0x28);
    irq_install(0, timer_handler);   /* IRQ0 = timer */
    sti();

    for(;;) { __asm__ __volatile__("hlt"); }
}
