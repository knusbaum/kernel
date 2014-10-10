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
#include "common.h"
#include "paging.h"
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

void general_protection(registers_t regs) {
    terminal_writestring("General Protection Fault. Code: ");
    terminal_write_dec(regs.err_code);
    panic("Halting.\n");
}

void invalid_opcode(registers_t regs){
    terminal_writestring("Invalid Opcode. Code: ");
    terminal_write_dec(regs.err_code);
    panic("Halting.\n");
}

void kernel_main()
{
  terminal_initialize();
  init_gdt();
  
  remap_pic();
  init_idt();

  register_interrupt_handler(13, general_protection);
  register_interrupt_handler(6, general_protection);

  init_timer(100);

  terminal_writestring("Hello, kernel World!\nHello, Again!\n");
  //terminal_setcolor(make_color(COLOR_RED, COLOR_BLUE));
  terminal_writestring("Right now I don't do much. Soon I'll have a few things to do.\n");
  
  
  asm volatile ("int $0x3");
  asm volatile ("int $0x4");
  //  asm volatile ("int $32");


  initialize_paging();

  terminal_writestring("Paging initialized.\n");

  terminal_write_hex(0x01010101);
  terminal_writestring("\n");
  terminal_write_hex(0xFFFFFFFF);
  terminal_writestring("\n");
  terminal_write_hex(0xDEADBEEF);

  terminal_writestring("\nDone writing hex.\n");


  ASSERT(1 == 0);

//  uint32_t *ptr = (uint32_t*)0xA0000000;
//  uint32_t do_page_fault = *ptr;
//  int i = do_page_fault;
  int i = 0;
  while(1){
    i++;
    //    terminal_write_dec(i);
    
  }
  terminal_write_dec(i);

}
