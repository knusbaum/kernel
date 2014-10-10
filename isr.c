#include "isr.h"
#include "terminal.h"
#include "pic.h"
#include "port.h"
#include <stdint.h>

extern void halt();

isr_handler_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t interrupt, isr_handler_t handler)
{
  interrupt_handlers[interrupt] = handler;
}

void isr_handler(registers_t regs)
{
    if(regs.int_no == 13)
    {
        terminal_writestring("General Protection Fault. Code: ");
        terminal_write_dec(regs.err_code);
//        halt();
    }
    else if(regs.int_no == 14)
    {
        terminal_writestring("Page Fault.\n");
        if( !(regs.err_code && 0x01)){
            terminal_writestring("    Page not present.\n");
        }
        
        if(regs.err_code && 0x02){
            terminal_writestring("    Write Error.\n");
        }
        else {
            terminal_writestring("    Read Error.\n");
        }
        
        if(regs.err_code && 0x04){
            terminal_writestring("    User Mode.\n");
        }
        else {
            terminal_writestring("    Kernel Mode.\n");
        }

        if(regs.err_code && 0x08){
            terminal_writestring("    Reserved bits overwritten.\n");
        }
        
        if(regs.err_code && 0x10){
            terminal_writestring("    Fault during instruction fetch.\n");
        }
    }
  
    terminal_writestring("Received interrupt: ");
    terminal_write_dec(regs.int_no);
    terminal_writestring("\n");
      
}


void irq_handler(registers_t regs)
{

  //terminal_writestring("Received IRQ: ");
  //terminal_write_dec(regs.int_no);
  //terminal_writestring("\n");

  //If int_no >= 40, we must reset the slave as well as the master
  if(regs.int_no >= 40)
    {
      //reset slave
      outb(SLAVE_COMMAND, PIC_RESET);
    }
  
  outb(MASTER_COMMAND, PIC_RESET);

  if(interrupt_handlers[regs.int_no])
    {
      interrupt_handlers[regs.int_no](regs);
    }

}

