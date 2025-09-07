//
// Created by T480 on 07.09.2025.
//

#ifndef KBD_H
#define KBD_H

#include <stdint.h>
#include <stdbool.h>

/* Initialize driver (installs IRQ1 handler). */
void kbd_init(void);

/* Non-blocking: returns true if a char is available; out param set to char. */
bool kbd_getch_nonblock(char* out);

/* Blocking: spin until a char is available (uses hlt). */
char kbd_getch(void);

#endif //KBD_H
