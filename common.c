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
    terminal_set_cursor(0, 1);
    terminal_setcolor(make_color(COLOR_DARK_GREY, COLOR_BLACK));
    terminal_settextcolor(make_color(COLOR_RED, COLOR_BLACK));
    terminal_writestring("PANIC: ");
    terminal_writestring(err);
    halt();
}
