//
// Created by T480 on 07.09.2025.
//

#include "pmm.h"
#include "mb2.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MAX_SUPPORTED_PHYS (1ull<<30) /* 1 GiB for v1; grow later */
#define MAX_PAGES (MAX_SUPPORTED_PHYS / PAGE_SIZE)
#define BITMAP_BYTES (MAX_PAGES / 8)

static volatile uint8_t bitmap[BITMAP_BYTES];
static size_t total_pages = 0;
static size_t free_pages_ = 0;

static inline size_t page_index(uintptr_t phys){ return (size_t)(phys / PAGE_SIZE); }
static inline void set_bit(size_t i){ bitmap[i>>3] |=  (uint8_t)(1u << (i&7)); }
static inline void clr_bit(size_t i){ bitmap[i>>3] &= (uint8_t)~(1u << (i&7)); }
static inline int  is_set(size_t i){ return (bitmap[i>>3] >> (i&7)) & 1u; }

static uintptr_t align_up(uintptr_t x, uintptr_t a){ return (x + a - 1) & ~(a - 1); }
static uintptr_t align_down(uintptr_t x, uintptr_t a){ return x & ~(a - 1); }

static void reserve_range(uintptr_t start, uintptr_t end) {
    if (end <= start) return;
    if (start >= MAX_SUPPORTED_PHYS) return;
    if (end > MAX_SUPPORTED_PHYS) end = MAX_SUPPORTED_PHYS;

    size_t i0 = page_index(align_down(start, PAGE_SIZE));
    size_t i1 = page_index(align_up(end, PAGE_SIZE));
    if (i1 > MAX_PAGES) i1 = MAX_PAGES;

    for (size_t i = i0; i < i1; ++i) {
        if (!is_set(i)) { set_bit(i); if (free_pages_) free_pages_--; }
    }
}


void pmm_init(uint64_t mb2_info, uintptr_t kernel_start, uintptr_t kernel_end) {
    /* Start with all pages reserved */
    for (size_t i = 0; i < BITMAP_BYTES; ++i) bitmap[i] = 0xFF;
    free_pages_ = 0;

    /* free only usable ranges (clipped to our 1 GiB window) */
    mb2_usable_range ranges[64];
    size_t n = mb2_get_usable_ranges(mb2_info, ranges, 64);

    for (size_t k = 0; k < n; ++k) {
        uintptr_t s = (uintptr_t)ranges[k].addr;
        uintptr_t e = (uintptr_t)(ranges[k].addr + ranges[k].len);
        if (s >= MAX_SUPPORTED_PHYS) continue;
        if (e >  MAX_SUPPORTED_PHYS) e = MAX_SUPPORTED_PHYS;

        uintptr_t s_al = align_up(s, PAGE_SIZE);
        uintptr_t e_al = align_down(e, PAGE_SIZE);
        if (e_al <= s_al) continue;

        size_t i0 = page_index(s_al), i1 = page_index(e_al);
        if (i1 > MAX_PAGES) i1 = MAX_PAGES;

        for (size_t i = i0; i < i1; ++i) {
            if (is_set(i)) { clr_bit(i); if (free_pages_ < MAX_PAGES) free_pages_++; }
        }
    }

    /* guard pages and kernel image */
    reserve_range(0, 0x1000);           /* null page */
    reserve_range(kernel_start, kernel_end);        /* kernel */
    reserve_range((uintptr_t)bitmap, (uintptr_t)bitmap + sizeof(bitmap)); /* bitmap storage */
}

uintptr_t pmm_alloc(void) {
    for (size_t i = 0; i < MAX_PAGES; ++i) {
        if (!is_set(i)) { set_bit(i); if (free_pages_) free_pages_--; return (uintptr_t)(i * PAGE_SIZE); }
    }
    return 0;
}

void pmm_free(uintptr_t phys) {
    if (phys & (PAGE_SIZE-1)) return;
    size_t i = page_index(phys);
    if (i >= MAX_PAGES) return;
    if (is_set(i)) { clr_bit(i); free_pages_++; }
}

size_t pmm_total_pages(void){ return total_pages; }
size_t pmm_free_pages(void){ return free_pages_; }

