/**
 * This file contains the minimal C code for dispatching
 * interrupts to handlers.
 */

#include "isr.h"
#include "idt.h"
#include "kernio.h"
#include "pic.h"
#include "port.h"
#include "stdint.h"

extern void halt();

isr_handler_t interrupt_handlers[256];

void register_interrupt_handler(uint8_t interrupt, isr_handler_t handler)
{
    interrupt_handlers[interrupt] = handler;
}

void isr_handler(registers_t regs)
{
    if(regs.int_no == GENERAL_PROTECTION_FAULT)
    {
        printf("General Protection Fault. Code: %d", regs.err_code);
        PANIC("General Protection Fault!");
    }

    if(interrupt_handlers[regs.int_no])
    {
        //printf("Handling %d!\n", regs.int_no);
        interrupt_handlers[regs.int_no](regs);
        //printf("Returning!\n");
    }
//    else {
//        printf("Got ISR.\n");
//        PANIC("Unhandled ISR.\n");
//    }
}


void irq_handler(registers_t regs)
{
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
//    else {
//        if(regs.int_no == 33) return;
//        printf("Got IRQ.\n");
//        char buff[1024];
//        sprintf(buff, "Unhandled IRQ %d\n", regs.int_no);
//        PANIC(buff);
//    }
}
