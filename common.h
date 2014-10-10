#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include "terminal.h"

void halt(void);
void *memset(void *p, int c, size_t count);

static inline void panic(char *reason) {
    terminal_writestring(reason);
    halt();
}

#define ASSERT(x) do { if(!(x)) { terminal_writestring("\nLine: "); terminal_write_dec(__LINE__); terminal_writestring(" File: "); terminal_writestring(__FILE__); panic("\nAssert failed."); } } while(0);

#endif
