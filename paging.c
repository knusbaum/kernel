#include "paging.h"
#include "frame.h"
#include "kheap.h"

page_directory_t * kernel_directory;
page_directory_t * current_directory;
extern uint32_t placement_address;

void initialize_paging()
{
    // 16 MB
    uint32_t mem_end_page = 0x1000000;
    nframes = mem_end_page / 0x1000;

    terminal_writestring("Grabbing ");
    terminal_write_dec(nframes);
    terminal_writestring(" frames.\n");
    terminal_writestring("Grabbing ");
    terminal_write_dec(INDEX_FROM_BIT(nframes) * sizeof(uint32_t));
    terminal_writestring(" bytes for bitmap.\n");

    frames = (uint32_t*)kmalloc(INDEX_FROM_BIT(nframes) * sizeof(uint32_t));
    memset(frames, 0, INDEX_FROM_BIT(nframes) * sizeof(uint32_t));
    kernel_directory = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
    memset(kernel_directory, 0, sizeof(page_directory_t));
    current_directory = kernel_directory;



    // Going to identity map from 0x0 to the end of used memory.
    uint32_t i = 0;
    while(i < placement_address)
    {
        alloc_frame(get_page(i, 1, kernel_directory), 0, 0);
        i += 0x1000;
        terminal_writestring("Allocating frame: ");
        terminal_write_hex(i);
        terminal_writestring("\n");
    }

    terminal_writestring("Done allocating frames.\n");

    register_interrupt_handler(14, page_fault);

    switch_page_directory(kernel_directory);
    
}

void switch_page_directory(page_directory_t *dir)
{
    current_directory = dir;
    asm volatile("mov %0, %%cr3":: "r"(&dir->tablesPhysical));
    
    uint32_t cr0 = 0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

page_t *get_page(uint32_t address, int make, page_directory_t *dir)
{
    // get index
    address /= 0x1000;
    
    // Find the page table containing this address.
    uint32_t table_index = address/1024;
    if (dir->tables[table_index]) // This table is already present
    {
        return &dir->tables[table_index]->pages[address%1024];
    }
    else if(make) {
        uint32_t temp;
        dir->tables[table_index] = (page_table_t*)kmalloc_ap(sizeof(page_table_t), &temp);
        memset(dir->tables[table_index], 0, 0x1000);
        dir->tablesPhysical[table_index] = temp | 0x7;
        return &dir->tables[table_index]->pages[address%1024];
    }
    else
    {
        return 0;
    }
}

void page_fault(registers_t regs)
{
    terminal_writestring("(");
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

    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

    terminal_writestring("    Faulting address: ");
    terminal_write_hex(faulting_address);
    terminal_writestring(" (");
    terminal_write_dec(faulting_address);
    terminal_writestring(")");
    panic(")\nPage Fault.\n");

//    uint32_t faulting_address;
//    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
//    
//    // The error code gives us details of what happened.
//    int present   = !(regs.err_code & 0x1); // Page not present
//    int rw = regs.err_code & 0x2;           // Write operation?
//    int us = regs.err_code & 0x4;           // Processor was in user-mode?
//    int reserved = regs.err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
//    int id = regs.err_code & 0x10;          // Caused by an instruction fetch?
//    
//    // Output an error message.
//    terminal_writestring("Page fault! ( ");
//    if (present) {terminal_writestring("present ");}
//    if (rw) {terminal_writestring("read-only ");}
//    if (us) {terminal_writestring("user-mode ");}
//    if (reserved) {terminal_writestring("reserved ");}
//    terminal_writestring(") at 0x");
//    terminal_write_dec(faulting_address);
//    terminal_writestring("\n");

}
