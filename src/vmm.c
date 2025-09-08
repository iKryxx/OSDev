//
// Created by T480 on 08.09.2025.
//

#include "vmm.h"
#include "pmm.h"
#include <stdint.h>
#include <string.h>

/* Exported from boot.S; identity-mapped */
extern uint64_t pml4[];

static void *os_memset(void *_Dst, int _Val, size_t _Size) {
    volatile unsigned char *Dst = (unsigned char *)_Dst;
    while (_Size > 0) {
        *Dst = (unsigned char)_Val;
        Dst++;
        _Size--;
    }
    return _Dst;
}

static inline void invlpg(uint64_t va) {
    __asm__ __volatile__("invlpg (%0)" : : "r"(va) : "memory");
}

static inline uint64_t* va_from_phys(uint64_t pa) {
    /* identity map for <1 GiB, so PA==VA here */
    return (uint64_t*)(uintptr_t)pa;
}

static uint64_t* get_or_alloc_table(uint64_t* table, unsigned idx, int alloc)
{
    uint64_t e = table[idx];
    if ((e & VMM_P) == 0) {
        if (!alloc) return 0;
        uint64_t pa = pmm_alloc();
        if (!pa) return 0;
        /* zero the new table (512 entries) */
        os_memset(va_from_phys(pa), 0, 4096);
        /* present + writable */
        table[idx] = (pa & ~0xFFFULL) | VMM_P | VMM_W;
        return va_from_phys(pa);
    } else {
        uint64_t pa = e & ~0xFFFULL;
        return va_from_phys(pa);
    }
}

void vmm_init(void) {
    /* nothing to do yet; we directly use exported pml4[] */
}

/* Split virt address into indices for each level */
static inline void split_va(uint64_t va, unsigned* i4, unsigned* i3, unsigned* i2, unsigned* i1) {
    *i1 = (va >> 12) & 0x1FF;
    *i2 = (va >> 21) & 0x1FF;
    *i3 = (va >> 30) & 0x1FF;
    *i4 = (va >> 39) & 0x1FF;
}

bool vmm_map(uint64_t va, uint64_t pa, uint64_t flags)
{
    unsigned i4,i3,i2,i1; split_va(va, &i4,&i3,&i2,&i1);


    uint64_t* p4 = pml4;
    uint64_t* p3 = get_or_alloc_table(p4, i4, 1); if (!p3) return false;
    uint64_t* p2 = get_or_alloc_table(p3, i3, 1); if (!p2) return false;

    /* If the PD entry is a 2MiB huge page, we’d need to split it.
       Our identity map set huge pages for 0..1GiB; but we’re mapping the heap
       outside that region, so this path won’t hit huge pages. */
    uint64_t e2 = p2[i2];
    if (e2 & VMM_PS) {
        /* should not happen for heap region we choose; bail if it does */
        return false;
    }
    uint64_t* p1 = get_or_alloc_table(p2, i2, 1); if (!p1) return false;

    /* final 4KiB entry */
    p1[i1] = (pa & ~0xFFFULL) | (flags | VMM_P);
    invlpg(va);
    return true;
}

bool vmm_unmap(uint64_t va)
{
    unsigned i4,i3,i2,i1; split_va(va, &i4,&i3,&i2,&i1);

    uint64_t* p4 = pml4;
    uint64_t e3 = p4[i4]; if ((e3 & VMM_P)==0) return false;
    uint64_t* p3 = va_from_phys(e3 & ~0xFFFULL);

    uint64_t e2 = p3[i3]; if ((e2 & VMM_P)==0) return false;
    uint64_t* p2 = va_from_phys(e2 & ~0xFFFULL);

    uint64_t e1 = p2[i2]; if ((e1 & VMM_P)==0 || (e1 & VMM_PS)) return false;
    uint64_t* p1 = va_from_phys(e1 & ~0xFFFULL);

    p1[i1] = 0;
    invlpg(va);
    return true;
}

bool vmm_query(uint64_t va, uint64_t* out_phys, uint64_t* out_flags)
{
    unsigned i4,i3,i2,i1; split_va(va, &i4,&i3,&i2,&i1);

    uint64_t e3 = pml4[i4]; if ((e3 & VMM_P)==0) return false;
    uint64_t* p3 = va_from_phys(e3 & ~0xFFFULL);

    uint64_t e2 = p3[i3]; if ((e2 & VMM_P)==0) return false;
    uint64_t* p2 = va_from_phys(e2 & ~0xFFFULL);

    uint64_t e1 = p2[i2]; if ((e1 & VMM_P)==0) return false;
    if (e1 & VMM_PS) {
        uint64_t base = e1 & ~0x1FFFFFULL; /* 2MiB mask */
        if (out_phys)  *out_phys  = base + (va & 0x1FFFFFULL);
        if (out_flags) *out_flags = e1 & 0xFFFULL;
        return true;
    }
    uint64_t* p1 = va_from_phys(e1 & ~0xFFFULL);
    uint64_t e0 = p1[i1]; if ((e0 & VMM_P)==0) return false;
    if (out_phys)  *out_phys  = (e0 & ~0xFFFULL) + (va & 0xFFF);
    if (out_flags) *out_flags = e0 & 0xFFFULL;
    return true;
}
