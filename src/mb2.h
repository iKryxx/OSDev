//
// Created by T480 on 07.09.2025.
//

#ifndef MB2_H
#define MB2_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    uint64_t addr;   /* physical start */
    uint64_t len;    /* length in bytes */
} mb2_usable_range;

/* Parse the Multiboot2 info at 'mb2_info' and fill up to 'max' usable ranges.
   Returns count of usable ranges found. */
size_t mb2_get_usable_ranges(uint64_t mb2_info, mb2_usable_range* out, size_t max);

#endif //MB2_H
