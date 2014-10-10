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
//    terminal_writestring("Received interrupt: ");
//    terminal_write_dec(regs.int_no);
//    terminal_writestring("\n");
    if(interrupt_handlers[regs.int_no])
    {
        interrupt_handlers[regs.int_no](regs);
    }
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

