#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>

//Standard allocate
uint32_t kmalloc(uint32_t size);

// Page Aligned
uint32_t kmalloc_a(uint32_t size);

// Get Physical Address
uint32_t kmalloc_p(uint32_t size, uint32_t *phys);

// Aligned with physical address
uint32_t kmalloc_ap(uint32_t size, uint32_t *phys);

#endif
