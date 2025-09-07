    .intel_syntax noprefix
    .text
    .global _load_gdt

gdt64:
    /* null */
    .quad 0
    /* code: base=0 limit=0xFFFFF, code64 */
    .quad 0x00AF9A000000FFFF
    /* data: */
    .quad 0x00AF92000000FFFF

gdt64_desc:
    .word (3*8 - 1)
    .quad gdt64

_load_gdt:
    lgdt [gdt64_desc]
    ret
