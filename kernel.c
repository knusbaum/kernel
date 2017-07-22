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
#include "fat32_console.h"
#include "kernio.h"
#include "vesa.h"

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
    terminal_initialize(make_color(COLOR_DARK_GREY, COLOR_LIGHT_GREY));
    terminal_set_status_color(make_color(COLOR_WHITE, COLOR_BLACK));
    printf("Booting kernel.\nHave: %d bytes of memory above 1MiB.\n", mi->mem_upper * 1024);

    //int32_test();
    //set_320x200x256();
    // full screen with blue color (1)
    //memset((char *)0xA0000, 1, (320*200));
    
    uint32_t low_pages = 256; // 1024 * 1024 bytes / 4096
    uint32_t high_pages = (mi->mem_upper * 1024) / 4096;

    uint32_t total_frames = high_pages + low_pages;

    init_gdt();

    remap_pic();
    init_idt();

    init_timer(100);

    initialize_keyboard();

    for(uint32_t i = 0; i < 0x5FFFFFFF; i++) {}
    
    set_vmode();
    uint32_t red = make_vesa_color(0xFF, 0, 0);
    uint32_t green = make_vesa_color(0, 0xFF, 0);
    uint32_t blue = make_vesa_color(0, 0, 0xFF);
    for(int y = 0; y < 10; y++) {
        for(int x = 0; x < 1280; x++) {
            draw_pixel_at(x, y, red);
            draw_pixel_at(x, y + 10, green);
            draw_pixel_at(x, y + 20, blue);
        }
    }
    return;
    
    initialize_paging(total_frames);
    
    malloc_stats();
    printf("Done setting up paging.\nKernel is ready to go!!!\n\n");
    terminal_settextcolor(make_color(COLOR_BLUE, COLOR_LIGHT_GREY));
    // Kernel ready to go!

    printf("Creating fat32 filesystem.\n");
    f32 *fs = makeFilesystem("");
    if(fs == NULL) {
        printf("Failed to create fat32 filesystem. Disk may be corrupt.\n");
        return;
    }
    printf("Starting fat32 console.\n");
    
    fat32_console(fs);

    printf("FAT32 shell exited. It is safe to power off.\nSystem is in free-typing mode.\n");

    while(1) {
        printf("Getting char.\n");
        char c = get_ascii_char();
        printf("%c", c);
    };
    printf("Halting.\n");

}
