//
// Created by T480 on 07.09.2025.
//

#include <stdint.h>
#include <stddef.h>

#include "idt.h"

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

void kernel_main(void) {
    /* light-grey on black = 0x07 */
    vga_clear(0x07);
    vga_puts_at("Booting Up OS...", 0, 0, 0x0F);
    vga_puts_at("Transition from GRUB to long mode complete", 1, 0, 0x0A);

    /* --- NEW: set up IDT for exceptions --- */
    idt_init();
    vga_puts_at("Initializing IDT complete", 2, 0, 0x0A);


    /* Optional test: trigger an exception to see the panic screen.
       Uncomment ONE of these lines to test.
    */
    //__asm__ __volatile__("ud2");        // invalid opcode (#UD)
    // __asm__ __volatile__("int3");       // breakpoint (#BP)
    // volatile int z = 0; int x = 1 / z;  // divide-by-zero (#DE)
    for(;;) { __asm__ __volatile__("hlt"); }
}
