//
// Created by T480 on 07.09.2025.
//

#ifndef PIC_H
#define PIC_H

#include <stdint.h>

void pic_remap(uint8_t offset1, uint8_t offset2);   // usually something like 0x20, 0x28
void pic_send_eoi(int irq);                         // irq = 0..15
void pix_disable(void);                             // optional helper

#endif //PIC_H
