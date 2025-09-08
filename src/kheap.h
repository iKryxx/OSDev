//
// Created by T480 on 08.09.2025.
//

#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>

void   kheap_init(uint64_t reserve_bytes);   /* optional pre-reserve, can be 0 */
void*  kmalloc(size_t size);                 /* 16-byte aligned */
void   kfree(void* ptr);                     /* no-op for now */

#endif //KHEAP_H
