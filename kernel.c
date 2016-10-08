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
#include "ata_pio_drv.h"

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
    int i = 0;

    terminal_writestring("Checking for primary ATA master drive: ");
    uint8_t res = identify();
    terminal_write_dec(res);
    if(res) {
        terminal_writestring("Drive present!\n");
    }
    else {
        terminal_writestring("Drive NOT present!\n");
        return;
    }

    uint8_t drive_buffer[256 * 2];
    ata_pio_read48(0, 1, drive_buffer);

    terminal_writestring("Data read! First byte: ");
    terminal_write_hex(((uint32_t *)drive_buffer)[0]);
    terminal_putchar('\n');

    terminal_writestring("Trying to write to drive!\n");
    for(i = 0; i < 256 * 2; i++) {
        drive_buffer[i] = 0xEF;
    }
    ata_pio_write48(0, 1, drive_buffer);
    terminal_writestring("Done writing!\n");

    ata_pio_read48(0, 1, drive_buffer);
    terminal_writestring("Data read! First byte: ");
    terminal_write_hex(((uint32_t *)drive_buffer)[0]);
    terminal_putchar('\n');
    return;

    // Spinloop to allow status to converge before halt.
    for(i = 0; i < 0x0FFFFFFF; i++){}
    while(1) {
        char c = get_ascii_char();
        terminal_putchar(c);
    };
    terminal_writestring("Halting.\n");

}
