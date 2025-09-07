//
// Created by T480 on 07.09.2025.
//

#include "mb2.h"

#include <stddef.h>
#include <stdint.h>

#pragma pack(push,1)
typedef struct {
    uint32_t total_size;
    uint32_t reserved;
} mb2_hdr;

typedef struct {
    uint32_t type;
    uint32_t size; /* includes this header */
} mb2_tag;

typedef struct {
    uint32_t type;  /* 6 for memory map */
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    /* entries follow */
} mb2_tag_mmap;

typedef struct {
    uint64_t addr;
    uint64_t len;
    uint32_t type;      /* 1=usable */
    uint32_t reserved;
} mb2_mmap_entry;
#pragma pack(pop)

static inline uint32_t align8(uint32_t x){ return (x + 7u) & ~7u; }

size_t mb2_get_usable_ranges(uint64_t mb2_info, mb2_usable_range* out, size_t max) {
    if (!mb2_info || !out || !max) return 0;
    const uint8_t* base = (const uint8_t*)(uintptr_t)mb2_info;
    const mb2_hdr* hdr  = (const mb2_hdr*)base;

    size_t found = 0;
    for (uint32_t off = 8; off + 8 <= hdr->total_size; off += align8(((mb2_tag*)(base+off))->size)) {
        const mb2_tag* tag = (const mb2_tag*)(base + off);
        if (tag->type == 0) break; /* end */
        if (tag->type == 6) {
            const mb2_tag_mmap* mm = (const mb2_tag_mmap*)tag;
            const uint8_t* p = (const uint8_t*)mm + sizeof(*mm);
            const uint8_t* end = (const uint8_t*)tag + tag->size;
            while (p + mm->entry_size <= end) {
                const mb2_mmap_entry* e = (const mb2_mmap_entry*)p;
                if (e->type == 1 && e->len) {
                    if (found < max) { out[found].addr = e->addr; out[found].len = e->len; }
                    found++;
                }
                p += mm->entry_size;
            }
        }
    }
    return (found > max ? max : found);
}