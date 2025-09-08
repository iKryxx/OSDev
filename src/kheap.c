//
// Created by T480 on 08.09.2025.
//

#include "kheap.h"
#include "vmm.h"
#include "pmm.h"
#include <stdint.h>
#include <string.h>

#define HEAP_BASE  0x0000100000000000ull  /* 1 TB, unmapped in your setup */
#define PAGE       4096ull
#define ALIGN_UP(x,a)   (((x)+((a)-1)) & ~((a)-1))

static uint64_t heap_cur   = HEAP_BASE;
static uint64_t heap_mapped_end = HEAP_BASE;

static void *os_memset(void *_Dst, int _Val, size_t _Size) {
    volatile unsigned char *Dst = (unsigned char *)_Dst;
    while (_Size > 0) {
        *Dst = (unsigned char)_Val;
        Dst++;
        _Size--;
    }
    return _Dst;
}
static int map_one_page(uint64_t va)
{
    uint64_t pa = pmm_alloc();
    if (!pa) return 0;
    /* zero the physical page (identity ≤1GiB) */
    os_memset((void*)(uintptr_t)pa, 0, PAGE);
    return vmm_map(va, pa, VMM_W) ? 1 : 0;
}

static int ensure_mapped_until(uint64_t va_end)
{
    while (heap_mapped_end < va_end) {
        if (!map_one_page(heap_mapped_end)) return 0;
        heap_mapped_end += PAGE;
    }
    return 1;
}

void kheap_init(uint64_t reserve_bytes)
{
    heap_cur = HEAP_BASE;
    heap_mapped_end = HEAP_BASE;
    if (reserve_bytes) {
        uint64_t need = HEAP_BASE + ALIGN_UP(reserve_bytes, PAGE);
        ensure_mapped_until(need);
    }
}

void* kmalloc(size_t sz)
{
    if (sz == 0) sz = 1;
    /* simple 16-byte alignment */
    heap_cur = ALIGN_UP(heap_cur, 16);
    uint64_t end = heap_cur + sz;
    if (!ensure_mapped_until(ALIGN_UP(end, PAGE))) return 0;
    void* ptr = (void*)(uintptr_t)heap_cur;
    heap_cur = end;
    return ptr;
}

void kfree(void* ptr) { (void)ptr; /* TODO: implement free list later */ }

