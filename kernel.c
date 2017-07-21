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
#include "realmode.h"

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif


#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif

extern void pause();
extern char _binary_f32_disk_start;

// int32 test
void int32_test()
{
    int y;
    regs16_t regs;
     
    // switch to 320x200x256 graphics mode
    regs.ax = 0x0013;
    int32(0x10, &regs);
     
    // full screen with blue color (1)
    memset((char *)0xA0000, 1, (320*200));
     
    // draw horizontal line from 100,80 to 100,240 in multiple colors
    for(y = 0; y < 200; y++)
        memset((char *)0xA0000 + (y*320+80), y, 160);
     
    // wait for key
    regs.ax = 0x0000;
    int32(0x16, &regs);
     
    // switch to 80x25x16 text mode
    regs.ax = 0x0003;
    int32(0x10, &regs);
}

void set_320x200x256() {
    // switch to 320x200x256 graphics mode
    regs16_t regs;
    regs.ax = 0x0013;
    int32(0x10, &regs);
}

static inline void draw_pixel_at(int x, int y, char color) {
    memset((char *)0xA0000 + (y*320) + x, color, 1);
}

void kernel_main(struct multiboot_info *mi)
{
    terminal_initialize(make_color(COLOR_DARK_GREY, COLOR_LIGHT_GREY));
    terminal_set_status_color(make_color(COLOR_WHITE, COLOR_BLACK));
    printf("Booting kernel.\nHave: %d bytes of memory above 1MiB.\n", mi->mem_upper * 1024);

    //int32_test();
    set_320x200x256();
    // full screen with blue color (1)
    memset((char *)0xA0000, 1, (320*200));
    
    uint32_t low_pages = 256; // 1024 * 1024 bytes / 4096
    uint32_t high_pages = (mi->mem_upper * 1024) / 4096;

    uint32_t total_frames = high_pages + low_pages;

    init_gdt();

    remap_pic();
    init_idt();

    init_timer(100);

    initialize_keyboard();

    initialize_paging(total_frames);

    draw_pixel_at(319,199,2);
    for(int c = 0; c < 256; c++) {
        for(int x = 0; x < 320; x++) {
            for(int y = 0; y < 200; y++) {
                draw_pixel_at(x, y, c);
            }
        }
        for(unsigned int i = 0; i < 0xFFFFFF; i++) {
            
        }
    }
    draw_pixel_at(160, 100, 10);
    return;
    
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
