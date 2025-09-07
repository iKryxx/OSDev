//
// Created by T480 on 07.09.2025.
//

#include "irq.h"
#include "pic.h"

static irq_handler_t handlers[16];
static void*        contexts[16];

void irq_register(int irq, irq_handler_t fn, void* ctx) {
    if (irq < 0 || irq >= 16) return;
    handlers[irq] = fn;
    contexts[irq] = ctx;
}

/* Dispatch IRQs. Called from isr_common_handler in idt.c */
void irq_common_handler(const regs_t* r) {
    int vector = (int)r->int_no;
    if (vector < 32 || vector > 47) return;

    int irq = vector - 32;
    if (handlers[irq]) {
        handlers[irq](contexts[irq], r);
    }

    pic_send_eoi(irq);
}
