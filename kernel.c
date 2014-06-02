#if !defined(__cplusplus)
#include <stdbool.h> /* C doesn't have booleans by default. */
#endif
#include <stddef.h>
#include <stdint.h>
#include "terminal.h"
//#include "port.h"
 
/* Check if the compiler thinks if we are targeting the wrong operating system. */
//#if defined(__linux__)
//#error "You are not using a cross-compiler, you will most certainly run into trouble"
//#endif
 
/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

 
#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif

/*
void outb(uint16_t port, uint8_t value)
{
  asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

uint8_t inb(uint16_t port)
{
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
  return ret;
}

uint16_t inw(uint16_t port)
{
  uint16_t ret;
  asm volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
  return ret;
} 
/**/

void kernel_main()
{
  //char *letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n";
  terminal_initialize();
  /* Since there is no support for newlines in terminal_putchar yet, \n will
     produce some VGA specific character instead. This is normal. */
  //  move_cursor(10,10);
  //terminal_putchar('c');
  //  call_putchar();
  //  move_cursor(10,10);
  terminal_writestring("Hello, kernel World!\nHello, Again!\n");
  //terminal_setcolor(make_color(COLOR_RED, COLOR_BLUE));
  terminal_writestring("Right now I don't do much. Soon I'll have a few things to do.\n");
  for(int i = 0; i < 100; i++)
    {
      //      terminal_writestring(&letters[i % 5]);
    }
  char x[10];
  itos(1234, x);
  terminal_writestring(x);
  terminal_writestring("\n\n\n");
  terminal_writestring("hello another time\n");
}
