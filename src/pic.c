//
// Created by T480 on 07.09.2025.
//

#include "pic.h"
#include "io.h"

#define PIC1        0x20
#define PIC2        0xA0
#define PIC1_CMD    PIC1
#define PIC1_DATA   (PIC1+1)
#define PIC2_CMD    PIC2
#define PIC2_DATA   (PIC2+1)

#define PIC_EOI     0x20

#define ICW1_INIT   0x10
#define ICW1_ICW4   0x01
#define ICW4_8086   0x01

void pic_remap(uint8_t offset1, uint8_t offset2) {
    const uint8_t a1 = inb(PIC1_DATA);
    const uint8_t a2 = inb(PIC2_DATA);

    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);

    outb(PIC1_DATA, offset1); // vector offset for PIC1 (IRQs 0..7)
    outb(PIC2_DATA, offset2); // vector offset for PIC2 (IRQs 8..15)

    outb(PIC1_DATA, 0x04);    // tell master PIC there is a slave at IRQ2
    outb(PIC2_DATA, 0x02);    // tell slave PIC its cascade identity (IRQ2)

    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    // restore saved masks
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}


void pic_send_eoi(int irq) {
    if (irq >= 8) outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}

void pic_disable(void) {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}
