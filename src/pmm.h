//
// Created by T480 on 07.09.2025.
//

#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "mb2.h"

#define PAGE_SIZE 4096

/* Initialize with Multiboot2 usable ranges and kernel occupied range. */
void pmm_init(uint64_t mb2_info, uintptr_t kernel_start, uintptr_t kernel_end);

/* Allocate one 4KiB physical page. Returns 0 on failure. */
uintptr_t pmm_alloc(void);

/* Free a previously allocated page (must be page-aligned). */
void pmm_free(uintptr_t phys);

/* Stats */
size_t pmm_total_pages(void);
size_t pmm_free_pages(void);

#endif //PMM_H
