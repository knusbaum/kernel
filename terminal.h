#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>

/* Hardware text mode color constants. */
enum vga_color
  {
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_GREEN = 2,
    COLOR_CYAN = 3,
    COLOR_RED = 4,
    COLOR_MAGENTA = 5,
    COLOR_BROWN = 6,
    COLOR_LIGHT_GREY = 7,
    COLOR_DARK_GREY = 8,
    COLOR_LIGHT_BLUE = 9,
    COLOR_LIGHT_GREEN = 10,
    COLOR_LIGHT_CYAN = 11,
    COLOR_LIGHT_RED = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_LIGHT_BROWN = 14,
    COLOR_WHITE = 15,
  };
 
uint8_t make_color(enum vga_color fg, enum vga_color bg); 
size_t strlen(const char* str);
char * itos(uint32_t myint, char buffer[], int bufflen);
 
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
 
void terminal_initialize();
void terminal_setcolor(uint8_t color);

void terminal_putchar(char c);
void terminal_writestring(const char* data);
void terminal_write_dec(uint32_t d);

#endif
