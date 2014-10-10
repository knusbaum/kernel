#ifndef PAGING_H
#define PAGING_H

#include "common.h"
#include "isr.h"

typedef struct page
{
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t accessed   : 1;
    uint32_t dirty      : 1;
    uint32_t unused     : 7;
    uint32_t frame      : 20;
} page_t;

typedef struct page_table
{
    page_t pages[1024];
} page_table_t;

typedef struct page_directory
{
    // Pointers to our page tables.
    page_table_t *tables[1024];

    // Pointers to the *physical* address of the page tables for CR3
    uint32_t tablesPhysical[1024];

    //Physical address of tablesPhysical.
    uint32_t physicalAddr;
} page_directory_t;

// Set up page directories, enable paging
void initialize_paging();

// Switch to a new page directory.
void switch_page_directory(page_directory_t *new);

// Get a pointer to the page required.
// If make == 1, and the page-table it belongs indoesn't exist, 
// create it.
page_t * get_page(uint32_t address, int make, page_directory_t *dir);

// Page fault handler
void page_fault(registers_t regs);

#endif
