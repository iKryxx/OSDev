//
// Created by T480 on 07.09.2025.
//

#include <stdint.h>
#include <stddef.h>

#include "idt.h"
#include "io.h"
#include "kbd.h"
#include "kheap.h"
#include "pic.h"
#include "pit.h"
#include "pmm.h"
#include "vmm.h"

static volatile uint16_t* const VGA = (uint16_t*)0xB8000;
static const int VGA_W = 80;
static const int VGA_H = 25;

extern char _kernel_start, _kernel_end;

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

static int cursor_row = 6, cursor_col = 0;
static void cursor_advance() {
    cursor_col++;
    if (cursor_col > VGA_W - 1) {
        cursor_col = cursor_row == VGA_H - 1 ? VGA_W : 0;
        cursor_row = cursor_row + 1 == VGA_H ? VGA_H - 1 : cursor_row + 1;
    }
}
static void cursor_retreat() {
    cursor_col--;
    if (cursor_col < 0) {
        cursor_col = cursor_row == 0 ? 0 : VGA_W - 1;
        cursor_row = cursor_row - 1 == -1 ? 0 : cursor_row - 1;
    }
}

static void putc_at(char c, int row, int col, uint8_t color) {
    VGA[row*VGA_W + col] = ((uint16_t)color<<8) | (uint8_t)c;
}
static void print_hex_at(uint64_t x, int row, int col, uint8_t color) {
    const char* H="0123456789ABCDEF";
    VGA[row*VGA_W + col++] = ((uint16_t)color<<8) | '0';
    VGA[row*VGA_W + col++] = ((uint16_t)color<<8) | 'x';
    for (int i=15;i>=0;i--){
        VGA[row*VGA_W + col++] = ((uint16_t)color<<8) | H[(x>>(i*4))&0xF];
    }
}
static void print_num_at(uint64_t x, int row, int col, uint8_t color) {
    char tmp[21]; int n=0;
    if (x==0) { putc_at('0', row, col, color); return; }
    while (x) { tmp[n++] = '0' + (x%10); x/=10; }
    for (int i=0;i<n;i++) putc_at(tmp[n-1-i], row, col+i, color);
}
static void putchar_cli(char c) {
    if (c=='\n') { cursor_col = 0; if (++cursor_row>=VGA_H) cursor_row=VGA_H-1; return; }
    if (c=='\b') { cursor_retreat(); putchar_cli(' '); cursor_retreat(); return;}
    if (c == '\t') {return; /* Not Implemented Yet!*/}
    VGA[cursor_row*VGA_W + cursor_col] = ((uint16_t)0x0F<<8) | (uint8_t)c;
    cursor_advance();
}

void kernel_main(uint64_t mb2_info) {
    /* light-grey on black = 0x07 */
    vga_clear(0x07);
    vga_puts_at("Booting Up OS...", 0, 0, 0x0F);
    vga_puts_at("Transition from GRUB to long mode complete", 1, 0, 0x0A);

    idt_init();
    vga_puts_at("Initializing IDT complete", 2, 0, 0x0A);

    pmm_init(mb2_info, (uintptr_t)&_kernel_start, (uintptr_t)&_kernel_end);
    vmm_init();
    kheap_init(0);
    vga_puts_at("Initializing PMM, VMM and KHeap complete", 3, 0, 0x0A);

    //char* s = (char*)kmalloc(64);
    char* s = NULL;
    for (int i=0;i<63;i++) s[i] = "hello from kheap! "[i % 20];
    s[63] = 0;
    s[70] = '4';
    vga_puts_at(s, 4, 0, 0x0A);

    pic_remap(0x20, 0x28);

    pit_init(1000);  /* 1 kHz tick */
    vga_puts_at("Initializing PIT complete", 5, 0, 0x0A);

    kbd_init();
    vga_puts_at("Initializing KBD complete", 6, 0, 0x0A);

    sti();           /* enable ints */
    for (;;) {
        char ch;
        if (kbd_getch_nonblock(&ch)) {
            putchar_cli(ch);
        } else {
            __asm__ __volatile__("hlt");  /* sleep until next IRQ */
        }
    }
}
