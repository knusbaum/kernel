#include "common.h"
#include "terminal.h"

extern void halt();

void *memset(void *p, int c, size_t count)
{
    for(unsigned int i = 0; i < count; i++)
    {
        ((char *)p)[i] = c;
    }
    return p;
}

void PANIC(char *err) {
    terminal_writestring("PANIC: ");
    terminal_writestring(err);
    halt();
}
