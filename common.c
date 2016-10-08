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

void *memcpy(void *dest, const void *src, size_t n) {

    char *cdest = dest;
    char *csrc = src;

    size_t i;
    for(i = 0; i < n; i++) {
        cdest[i] = csrc[i];
    }

    return dest;
}

int toupper(int c) {
    if(c >= 97 && c <= 122) {
        return c - 32;
    }
    return c;
}
int tolower(int c) {
    if(c >=65 && c <= 90) {
        return c + 32;
    }
    return c;
}

int strcmp(const char *s1, const char *s2) {
    while(*s1 && *s2) {
        if(*s1 != *s2) {
            return s1 - s2;
        }
        s1++;
        s2++;
    }
    if(*s1) return 1;
    if(*s2) return -1;
    return 0;
}

void PANIC(char *err) {
    terminal_set_cursor(0, 1);
    terminal_setcolor(make_color(COLOR_DARK_GREY, COLOR_BLACK));
    terminal_settextcolor(make_color(COLOR_RED, COLOR_BLACK));
    terminal_writestring("PANIC: ");
    terminal_writestring(err);
    halt();
}
