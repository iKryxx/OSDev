    .intel_syntax noprefix

/* ---------- Multiboot2 header ---------- */
    .section .multiboot2
    .align 8
    .long 0xE85250D6          /* magic */
    .long 0                   /* arch=i386 */
    .long header_end - header_start
    .long 0x100000000 - (0xE85250D6 + 0 + (header_end - header_start))
header_start:
    .word 0
    .word 0
    .long 8
    .align 8
header_end:

/* ---------- externs ---------- */
    .extern kernel_main

/* ---------- Multiboot 2 Info Pointer ---------- */

    .section .data
    .align 8
mb2_info_ptr:   .quad 0

/* ---------- 32-bit entry ---------- */
    .section .text
    .globl _start
    .type _start, @function

    .code32
_start:
    cli

    /* EBX holds Multiboot2 info pointer (physical) */
    mov [mb2_info_ptr], ebx

    /* Load our GDT (has a 64-bit code segment at selector 0x08) */
    lgdt [gdt_desc32]

    /* Identity-mapped paging for 0..1GiB */
    lea eax, [pml4]
    mov cr3, eax

    /* Enable PAE + PGE */
    mov eax, cr4
    or  eax, (1 << 5)         /* PAE */
    or  eax, (1 << 7)         /* PGE */
    mov cr4, eax

    /* Enable Long Mode in EFER */
    mov ecx, 0xC0000080       /* IA32_EFER */
    rdmsr
    or eax, (1 << 8)          /* LME */
    wrmsr

    /* Enable paging */
    mov eax, cr0
    or  eax, (1 << 31)        /* PG */
    mov cr0, eax

    /* Far jump to 64-bit code segment (selector 0x08) via push/retf */
    push 0x08                       /* new CS */
    lea  eax, [long_mode_entry]     /* 32-bit offset */
    push eax                        /* new RIP */
    retf

/* ---------- 64-bit code ---------- */
    .code64
long_mode_entry:
    /* Load flat data segment (selector 0x10) */
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    /* Stack */
    lea rsp, [stack_top]

    /* Pass Multiboot2 info pointer in RDI to kernel_main */
    mov rdi, qword ptr [mb2_info_ptr]

    /* Call C kernel */
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

/* ---------- Minimal GDT (with 64-bit code) ---------- */
    .align 8
gdt64:
    .quad 0                          /* null */
    /* code64: base=0, limit=~0, flags: G=1, D/B=0, L=1, AVL=0; access=0x9A */
    .quad 0x00AF9A000000FFFF
    /* data: base=0, limit=~0; access=0x92 (read/write) */
    .quad 0x00AF92000000FFFF

/* 6-byte GDTR for 32-bit lgdt (limit: size-1, base: 32-bit) */
gdt_desc32:
    .word (3*8 - 1)
    .long gdt64

/* ---------- Paging structures (identity map 0..1GiB with 2MiB pages) ---------- */
    .align 4096
    .global pml4
pml4:
    .quad pdpt + 0x003
    .fill 511,8,0

    .align 4096
pdpt:
    .quad pd + 0x003
    .fill 511,8,0

    .align 4096
pd:
    .set i,0
.rept 512
    .quad (i * 0x200000) + 0x083   /* P|W|PS */
    .set i, i+1
.endr

/* ---------- Stack ---------- */
    .bss
    .align 16
stack:
    .space 16384
stack_top:
