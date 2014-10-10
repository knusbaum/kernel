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
    kernel_directory = (page_directory_t*)kmalloc_a(sizeof(page_directory_t));
    memset(kernel_directory, 0, sizeof(page_directory_t));
    current_directory = kernel_directory;



    // Going to identity map from 0x0 to the end of used memory.
    uint32_t i = 0;
    while(i < placement_address)
    {
        alloc_frame(get_page(i, 1, kernel_directory), 0, 0);
        i += 0x1000;
    }



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
    panic(")\nPage Fault.\n");
}
