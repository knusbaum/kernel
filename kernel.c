#include "stdio.h"
#include "stddef.h"
#include "stdint.h"
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
#include "terminal.h"

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif


#if defined(__cplusplus)
extern "C" /* Use C linkage for kernel_main. */
#endif

extern void pause();
extern char _binary_f32_disk_start;

int main(void);


void kernel_main(struct multiboot_info *mi)
{
    terminal_initialize(COLOR_WHITE);
    uint32_t low_pages = 256; // 1024 * 1024 bytes / 4096
    uint32_t high_pages = (mi->mem_upper * 1024) / 4096;

    uint32_t total_frames = high_pages + low_pages;

    set_vmode();
    set_vesa_color(make_vesa_color(0x8F, 0x8F, 0x8F));
    init_gdt();

    remap_pic();
    init_idt();

    init_timer(100);

    initialize_keyboard();

    initialize_paging(total_frames, get_framebuffer_addr(), get_framebuffer_length());
    malloc_stats();
    printf("Done setting up paging.\n");

    setup_stdin();
    
    set_vesa_color(make_vesa_color(0xFF, 0xFF, 0xFF));
    printf("Kernel is ready to go!!!\n\n");

    // Kernel ready to go!

    printf("Creating fat32 filesystem.\n");
    master_fs = makeFilesystem("");
    if(master_fs == NULL) {
        printf("Failed to create fat32 filesystem. Disk may be corrupt.\n");
        return;
    }

    printf("Finding /foo/bar/baz/boo/dep/doo/poo/goo/tood.txt.\n");

    FILE *f = fopen("/foo/bar/baz/boo/dep/doo/poo/goo/tood.txt", NULL);
    if(f) {
        #define BCOUNT 1000
        uint8_t c[BCOUNT];
        printf("READING:.................................\n");
        int count, total;
        while((count = fread(&c, BCOUNT, 1, f)), count > 0) {
            for(int i = 0; i < count; i++) {
                printf("%c", c[i]);
            }
            total += count;
        }
        fclose(f);
        printf("Read %d bytes.\n", total);
    }
    else {
        printf("File not found. Continuing.\n");
    }

    printf("Starting fat32 console.\n");

    fat32_console(master_fs);

    printf("FAT32 shell exited.\n");

    //main();

    printf("LISP VM exited. It is safe to power off.\nSystem is in free-typing mode.\n");

    while(1) {
        char c = get_ascii_char();
        printf("%c", c);
    };
    printf("Halting.\n");

}
