//
// Created by T480 on 08.09.2025.
//

#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stdbool.h>

/* Page table bits (subset) */
enum {
    VMM_P = 1ull << 0,   /* present */
    VMM_W = 1ull << 1,   /* writable */
    VMM_U = 1ull << 2,   /* user */
    VMM_PWT = 1ull << 3,
    VMM_PCD = 1ull << 4,
    VMM_A = 1ull << 5,
    VMM_D = 1ull << 6,
    VMM_PS = 1ull << 7,  /* page size (set only in PD for 2MiB) */
    VMM_G = 1ull << 8,   /* global (ignore for now) */
    /* NX is in the high bit if enabled; we’ll ignore for simplicity */
};

void vmm_init(void);  /* picks up current CR3 / uses exported pml4 */
bool vmm_map(uint64_t virt, uint64_t phys, uint64_t flags);   /* 4KiB map */
bool vmm_unmap(uint64_t virt);                                /* 4KiB unmap */
bool vmm_query(uint64_t virt, uint64_t* out_phys, uint64_t* out_flags);

#endif //VMM_H
