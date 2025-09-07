//
// Created by T480 on 07.09.2025.
//

#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>
#include "idt.h"

/*
 * IRQ handler type.
 * - ctx:   user-supplied pointer (can be NULL)
 * - r:     snapshot of registers (regs_t) from the interrupt frame
 */
typedef void (*irq_handler_t)(void* ctx, const regs_t* r);

/* Register a handler for a given IRQ line (0..15). */
void irq_register(int irq, irq_handler_t fn, void* ctx);

/* Internal: called from isr_common_handler when int_no ∈ [32,47]. */
void irq_common_handler(const regs_t* r);

#endif //IRQ_H
