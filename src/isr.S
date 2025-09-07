.intel_syntax noprefix
.text
.code64

.extern isr_common_handler

/* save/restore general registers */
.macro PUSH_REGS
    /* push in this order so top-of-stack = r15 */
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
.endm

.macro POP_REGS
    /* pop mirrors PUSH: top was r15 */
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
.endm

/* Exceptions WITHOUT CPU error code: stack … [0][int_no] (int_no on top) */
.macro ISR_NOERR num
    .globl isr\num
    .type  isr\num, @function
isr\num:
    push 0              /* err_code (lower in memory after we push GPRs) */
    push \num           /* int_no   (will be directly below GPRs) */
    jmp isr_common
.endm

/* Exceptions WITH CPU error code: CPU already pushed err_code; we push int_no.
   Stack … [err_code][int_no]  (int_no on top) */
.macro ISR_ERR num
    .globl isr\num
    .type  isr\num, @function
isr\num:
    push \num
    jmp isr_common
.endm

.globl isr_common
isr_common:
    cli
    PUSH_REGS
    mov rdi, rsp                 /* rdi = &regs (our struct starts at r15) */
    call isr_common_handler
    POP_REGS
    add rsp, 16                  /* pop [int_no][err_code] */
    iretq

/* 0..31 */
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR   21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31
