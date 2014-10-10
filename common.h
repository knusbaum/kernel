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

#endif
