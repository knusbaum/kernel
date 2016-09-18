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

//! The symbol table for a.out format.
typedef struct {
    unsigned long tab_size;
    unsigned long str_size;
    unsigned long address;
    unsigned long reserved;
} aout_symbol_table_t;

//! The section header table for ELF format.
typedef struct {
    unsigned long num;
    unsigned long size;
    unsigned long address;
    unsigned long shndx;
} elf_section_header_table_t;

struct multiboot_info
{
    unsigned long flags;
    unsigned long mem_lower;
    unsigned long mem_upper;
    unsigned long boot_device;
    unsigned long cmdline;
    unsigned long mods_addr;
    union
    {
        aout_symbol_table_t aout_sym_t;
        elf_section_header_table_t elf_sec_t;
    } u;
    unsigned long mmap_length;
    unsigned long mmap_addr;
};


//extern uint32_t allocated_frames;

void kernel_main(struct multiboot_info *mi)
{
    terminal_initialize(make_color(COLOR_DARK_GREY, COLOR_WHITE));
    terminal_set_status_color(make_color(COLOR_WHITE, COLOR_BLACK));
    //terminal_writestring("Hello, kernel World!\nHello, Again!\n");
    terminal_writestring("Booting kernel.\n");
    terminal_writestring("Have: ");
    terminal_write_dec(mi->mem_upper * 1024);
    terminal_writestring(" bytes of memory above 1MiB.\n");

    uint32_t low_pages = 256; // 1024 * 1024 bytes / 4096
    uint32_t high_pages = (mi->mem_upper * 1024) / 4096;

    uint32_t total_frames = high_pages + low_pages;

    init_gdt();

    remap_pic();
    init_idt();

    init_timer(100);

    initialize_paging(total_frames);
    // There are some spinwaits in here for cinematic effect.
    // Just because it's fun to watch it do stuff.
    int i;
    for(i = 0; i < 0x0FFFFFFF; i++){}
    malloc_stats();
    terminal_writestring("Done setting up paging.\nKernel is ready to go!!!\n\n");
    for(i = 0; i < 0x0FFFFFFF; i++){}
    // Kernel ready to go!

    terminal_settextcolor(make_color(COLOR_BLUE, COLOR_WHITE));

    struct page_directory newdir;
    memset(&newdir, 0, sizeof newdir);
    terminal_writestring("Allocating a page for a new page directory!\n");
    struct page *p = get_page(0x0, 1, &newdir);
    terminal_writestring("Done. Freeing it.\n\n");
    kfree(p);

    terminal_writestring("Allocating a bunch of kheap!\n");
    void *ptrs2[4096];
    for(i = 0; i < 4096; i++) {
        ptrs2[i] = kmalloc(4096 * 10, 0, 0);
        int j;
        for(j = 0; j < 0x000FFFFF; j++) {}
    };

    terminal_writestring("Done!\n");
    malloc_stats();
    for(i = 0; i < 0x00FFFFFF; i++){}
    terminal_writestring("Freeing!\n");

    for(i = 4095; i >= 0; i--) {
        kfree(ptrs2[i]);
        int j;
        for(j = 0; j < 0x000FFFFF; j++) {}
    }
    malloc_stats();
    terminal_writestring("Done. Halting.\n");
}
