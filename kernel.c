#include <stdbool.h>
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
#include "kheap.h"
#include "multiboot.h"
#include "keyboard.h"
#include "fat32.h"

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif


#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif

extern void pause();
extern char _binary_f32_disk_start;

void kernel_main(struct multiboot_info *mi)
{
    terminal_initialize(make_color(COLOR_DARK_GREY, COLOR_WHITE));
    terminal_set_status_color(make_color(COLOR_WHITE, COLOR_BLACK));
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

    initialize_keyboard();

    initialize_paging(total_frames);

    malloc_stats();
    terminal_writestring("Done setting up paging.\nKernel is ready to go!!!\n\n");
    terminal_settextcolor(make_color(COLOR_BLUE, COLOR_WHITE));
    // Kernel ready to go!

    terminal_writestring("Creating fat32 filesystem.\n");
    f32 *fs = makeFilesystem("");
    if(fs == NULL) {
        terminal_writestring("Failed to create fat32 filesystem. Disk may be corrupt.\n");
        return;
    }

    terminal_writestring("Reading root directory!\n");
    struct directory root;
    populate_root_dir(fs, &root);
    terminal_writestring("Done! Going to print root.\n");
    print_directory(fs, &root);
    terminal_writestring("Done!\n");

    terminal_writestring("Reading subdir \"daz\"\n");
    populate_dir(fs, &root, root.entries[5].first_cluster);
    print_directory(fs, &root);
    terminal_writestring("Done!\n");

    while(1) {
        char c = get_ascii_char();
        terminal_putchar(c);
    };
    terminal_writestring("Halting.\n");

}
