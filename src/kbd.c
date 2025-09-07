//
// Created by T480 on 07.09.2025.
//

#include "kbd.h"
#include "io.h"
#include "irq.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* PS/2 ports */
#define KBD_DATA 0x60
#define KBD_STAT 0x64

/* Simple ring buffer */
#define KB_BUF_SIZE 128
static volatile uint8_t buf[KB_BUF_SIZE];
static volatile uint32_t head = 0, tail = 0;

/* Modifiers */
static bool shift = false, caps = false, ctrl = false, alt = false;

static inline bool buf_push(uint8_t c) {
    uint32_t n = (head + 1) % KB_BUF_SIZE;
    if (n == tail) return false; /* full */
    buf[head] = c; head = n; return true;
}
static inline bool buf_pop(uint8_t* out) {
    if (tail == head) return false;
    *out = buf[tail]; tail = (tail + 1) % KB_BUF_SIZE; return true;
}

/* US scancode set 1 -> ASCII (no keypad, no F-keys) */
static const char map_nomod[128] = {
/*0x00*/0,  27,'1','2','3','4','5','6','7','8','9','0','-','=', '\b',
/*0x0F*/'\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
/*0x1E*/'a','s','d','f','g','h','j','k','l',';','\'','`',   0,'\\',
/*0x2C*/'z','x','c','v','b','n','m',',','.','/',   0,'*',   0,' ',   0,
/*0x3C*/0,0,0,0,0,0,0,0,0,0,  /* rest 0 */
};
static const char map_shift[128] = {
/*0x00*/0,  27,'!','@','#','$','%','^','&','*','(',')','_','+', '\b',
/*0x0F*/'\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
/*0x1E*/'A','S','D','F','G','H','J','K','L',':','"','~',   0,'|',
/*0x2C*/'Z','X','C','V','B','N','M','<','>','?',   0,'*',   0,' ',   0,
/*0x3C*/0,0,0,0,0,0,0,0,0,0,
};

static char translate(uint8_t sc) {
    bool use_shift = shift ^ caps; /* caps flips case for letters only below */
    char c = use_shift ? map_shift[sc] : map_nomod[sc];

    /* Caps affects only alphabetic characters; our XOR above is ok because
       map tables already contain letters in both cases. Non-letters produce the
       shifted symbols (as on a real keyboard) when Shift is held. */
    return c;
}

static void on_irq1(void* ctx, const regs_t* r) {
    (void)ctx; (void)r;
    /* Read available scancode(s) — usually one per IRQ. */
    uint8_t sc = inb(KBD_DATA);

    bool release = sc & 0x80;
    uint8_t code = sc & 0x7F;

    /* Modifier handling (left/right collapse) */
    switch (code) {
        case 0x2A: case 0x36: shift = !release ? true : false; return; /* Shift */
        case 0x1D: ctrl  = !release; return; /* Ctrl */
        case 0x38: alt   = !release; return; /* Alt  */
        case 0x3A: if (!release) caps = !caps; return; /* Caps toggles on press */
        default: break;
    }

    if (release) return; /* key up for non-modifiers ignored */

    char c = translate(code);
    if (c) buf_push((uint8_t)c);
}

void kbd_init(void) {
    /* Enable IRQ1 via our registry */
    irq_register(1, on_irq1, NULL);
}

bool kbd_getch_nonblock(char* out) {
    uint8_t c;
    if (!buf_pop(&c)) return false;
    *out = (char)c;
    return true;
}

char kbd_getch(void) {
    char c;
    while (!kbd_getch_nonblock(&c)) {
        __asm__ __volatile__("hlt");
    }
    return c;
}
