//
// Created by T480 on 07.09.2025.
//

#include "idt.h"

#include <stddef.h>
#include <stdint.h>

#include "pic.h"

/* ------- minimal VGA printing (no libc) ------- */
static volatile uint16_t* const VGA = (uint16_t*)0xB8000;
static const int W = 80, H = 25;
static int cur_row = 0;

static inline uint16_t vga_entry(char c, uint8_t color) { return ((uint16_t)color << 8) | (uint8_t)c; }

static void kclear(uint8_t color) {
    uint16_t fill = ((uint16_t)color << 8) | ' ';
    for (int i = 0; i < W*H; ++i) VGA[i] = fill;
    cur_row = 0;
}
static void kputs(const char* s, uint8_t color) {
    int col = 0, row = cur_row;
    for (; *s; ++s) {
        if (*s == '\n' || col >= W) { col = 0; if (++row >= H) row = H-1; }
        if (*s != '\n') VGA[row*W + col++] = vga_entry(*s, color);
    }
    cur_row = row + 1;
}
static void kputhex64(uint64_t x, uint8_t color) {
    const char* hex = "0123456789ABCDEF";
    VGA[cur_row*W + 0] = vga_entry('0', color);
    VGA[cur_row*W + 1] = vga_entry('x', color);
    for (int i = 0; i < 16; ++i) {
        uint8_t nyb = (x >> ((15 - i) * 4)) & 0xF;
        VGA[cur_row*W + 2 + i] = vga_entry(hex[nyb], color);
    }
    cur_row++;
}

/* ------- IDT ------- */
static idt_entry_t idt[256];
static idt_ptr_t   idtr;

/* EXCEPTIONS */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);  extern void isr3(void);
extern void isr4(void);  extern void isr5(void);  extern void isr6(void);  extern void isr7(void);
extern void isr8(void);  extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void); extern void isr15(void);
extern void isr16(void); extern void isr17(void); extern void isr18(void); extern void isr19(void);
extern void isr20(void); extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void); extern void isr27(void);
extern void isr28(void); extern void isr29(void); extern void isr30(void); extern void isr31(void);

/* PIC */
extern void irq32(void); extern void irq33(void); extern void irq34(void); extern void irq35(void);
extern void irq36(void); extern void irq37(void); extern void irq38(void); extern void irq39(void);
extern void irq40(void); extern void irq41(void); extern void irq42(void); extern void irq43(void);
extern void irq44(void); extern void irq45(void); extern void irq46(void); extern void irq47(void);

/* per-IRQ (0..15) callbacks; arg is regs snapshot */
static void (*irq_handlers[16])(const regs_t*);

void irq_install(int irq, void (*fn)(const regs_t*)) {
    if (irq >= 0 && irq < 16) irq_handlers[irq] = fn;
}


static void idt_set_gate(int n, void* isr) {
    uint64_t addr = (uint64_t)isr;
    idt[n].offset_low  =  addr        & 0xFFFF;
    idt[n].selector    =  0x08;             /* matches your GDT code segment */
    idt[n].ist         =  0;                /* no IST yet */
    idt[n].type_attr   =  0x8E;             /* present, DPL=0, 64-bit interrupt gate */
    idt[n].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[n].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[n].zero        =  0;
}

static inline void lidt(const idt_ptr_t* p) {
    __asm__ __volatile__("lidt (%0)" : : "r"(p) : "memory");
}

/* exception names for nicer panic text */
static const char* EXC_NAME[32] = {
    "Divide-by-zero (#DE)","Debug (#DB)","NMI","Breakpoint (#BP)","Overflow (#OF)","Bound Range (#BR)",
    "Invalid Opcode (#UD)","Device Not Available (#NM)","Double Fault (#DF)","Coprocessor Overrun",
    "Invalid TSS","#NP Segment Not Present","#SS Stack Fault","#GP General Protection","#PF Page Fault",
    "Reserved","x87 FP","Alignment Check","Machine Check","SIMD FP","Virtualization",
    "Control Protection (#CP)","Reserved","Reserved","Reserved","Reserved","Reserved","Reserved","Reserved","Reserved","Reserved","Reserved"
};

void idt_init(void) {
    /* fill IDT with exception stubs */
    void (*stubs[32])(void) = {
        isr0,isr1,isr2,isr3,isr4,isr5,isr6,isr7,isr8,isr9,isr10,isr11,isr12,isr13,isr14,isr15,
        isr16,isr17,isr18,isr19,isr20,isr21,isr22,isr23,isr24,isr25,isr26,isr27,isr28,isr29,isr30,isr31
    };
    for (int i = 0; i < 32; ++i) idt_set_gate(i, (void*)stubs[i]);

    /* IRQs: CPU vectors 32..47 */
    void (*irqstubs[16])(void) = {
        irq32,irq33,irq34,irq35,irq36,irq37,irq38,irq39,
        irq40,irq41,irq42,irq43,irq44,irq45,irq46,irq47
    };
    for (int i = 0; i < 16; ++i) idt_set_gate(32+i, (void*)irqstubs[i]);

    idtr.limit = (uint16_t)(sizeof(idt) - 1);
    idtr.base  = (uint64_t)&idt[0];
    lidt(&idtr);
}

void irq_common_handler(const regs_t* r);

/* Called from isr_common in assembly with &regs in rdi */
void isr_common_handler(const regs_t* r) {
    /* If it is an IRQ (PIC), do not panic; dispatch instead */
    if (r->int_no >= 32 && r->int_no <= 47) {
        irq_common_handler(r);
        return;
    }

    kclear(0x07); /* grey/black */
    kputs("KERNEL PANIC: CPU exception\n", 0x0C);

    kputs("Vector: ", 0x0F);
    /* safe index */
    uint64_t n = r->int_no;
    const char* name = (n < 32) ? EXC_NAME[n] : "Unknown";
    kputs(name, 0x0F);
    kputs("\n", 0x0F);

    kputs("Error code: ", 0x0F);  kputhex64(r->err_code, 0x0A);
    kputs("RIP: ", 0x0F);         kputhex64(r->rip, 0x0A);
    kputs("CS: ", 0x0F);          kputhex64(r->cs, 0x0A);
    kputs("RFLAGS: ", 0x0F);      kputhex64(r->rflags, 0x0A);

    kputs("\nSystem halted.", 0x0C);
    for(;;) { __asm__ __volatile__("hlt"); }
}

void irq_common_handler(const regs_t* r) {
    uint64_t vec = r->int_no;
    if (vec >= 32 && vec <= 47) {
        int irq = (int)(vec - 32);
        if (irq_handlers[irq]) irq_handlers[irq](r);
        pic_send_eoi(irq);
    }
}