//
// Created by T480 on 07.09.2025.
//

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// 64-bit IDT entry (interrupt gate)

typedef struct __attribute__((packed)) {
    uint16_t offset_low;   // bits 0..15 of handler
    uint16_t selector;     // code segment selector (0x08)
    uint8_t  ist;          // bits 0..2 = IST index, rest zero
    uint8_t  type_attr;    // type + attributes (0x8E = present, DPL0, 64-bit interrupt gate)
    uint16_t offset_mid;   // bits 16..31 of handler
    uint32_t offset_high;  // bits 32..63 of handler
    uint32_t zero;
} idt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} idt_ptr_t;

/* The register snapshot we build in isr.S and hand to C */
typedef struct __attribute__((packed)) regs_t {
    /* pushed by our stub (general purpose) */
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rsi, rdi, rbp, rdx, rcx, rbx, rax;

    /* unified by our stub: always present in this order */
    uint64_t int_no;
    uint64_t err_code;

    /* pushed by CPU automatically */
    uint64_t rip, cs, rflags;
    uint64_t rsp, ss;       // valid if privilege level changed; otherwise undefined
} regs_t;

void idt_init(void);

#endif //IDT_H
