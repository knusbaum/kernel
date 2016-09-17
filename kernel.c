#if !defined(__cplusplus)
#include <stdbool.h> /* C doesn't have booleans by default. */
#endif
#include <stddef.h>
#include <stdint.h>
#include "terminal.h"
#include "gdt.h"
#include "idt.h"
#include "pit.h"
#include "pic.h"
#include "isr.h"
#include "paging.h"
#include "common.h"
//#include "kmalloc_early.h"
#include "kheap.h"

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

void kernel_main()
{
    terminal_initialize(make_color(COLOR_BLUE, COLOR_WHITE));
    terminal_writestring("Hello, kernel World!\nHello, Again!\n");

    init_gdt();

    remap_pic();
    init_idt();

    init_timer(100);

    initialize_paging();
    terminal_writestring("\nDone setting up paging.\n");
    malloc_stats();

    terminal_settextcolor(make_color(COLOR_WHITE, COLOR_BLACK));


    terminal_writestring("Performing allocations.\n");
    void *ptrs[300];
    int i = 0;
    for(i = 0; i < 300; i++) {
        ptrs[i] = kmalloc(1, 0, 0);
//        terminal_write_dec(i);
//        terminal_writestring(" ");
//        terminal_write_hex((uint32_t)x);
//        terminal_putchar('\n');
    }
    terminal_writestring("Done.\n");
    malloc_stats();
    terminal_writestring("Freeing.\n");
    for(i = 0; i < 300; i++) {
        kfree(ptrs[i]);
    }
    terminal_writestring("Done.\n");
    malloc_stats();

    terminal_writestring("Kmallocing page-aligned.\n");
    void *x = kmalloc(1, 1, 0);
    terminal_writestring("addr: ");
    terminal_write_hex((uint32_t)x);
    terminal_putchar('\n');

    x = kmalloc(1, 1, 0);
    terminal_writestring("addr: ");
    terminal_write_hex((uint32_t)x);
    terminal_putchar('\n');

    x = kmalloc(1, 1, 0);
    terminal_writestring("addr: ");
    terminal_write_hex((uint32_t)x);
    terminal_putchar('\n');

    x = kmalloc(1, 1, 0);
    terminal_writestring("addr: ");
    terminal_write_hex((uint32_t)x);
    terminal_putchar('\n');
}
