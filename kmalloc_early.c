#include "kmalloc_early.h"

extern uint32_t end_of_kernel;
uint32_t placement_address = (uint32_t)&end_of_kernel;

static uint32_t kmalloc_imp(uint32_t sz, int align, uint32_t *phys)
{
    if (align == 1 && (placement_address & 0xFFFFF000))
    {
        // If the address is not already page-aligned
        // Align it.
        placement_address &= 0xFFFFF000; // Set to current page
        placement_address += 0x1000;     // Advance one page
    }
    if (phys)
    {
        *phys = placement_address;
    }
    uint32_t tmp = placement_address;
    placement_address += sz;
    return tmp;
}

uint32_t kmalloc_a(uint32_t sz) {
    return kmalloc_imp(sz, 1, NULL);
}

uint32_t kmalloc_p(uint32_t sz, uint32_t *phys) {
    return kmalloc_imp(sz, 0, phys);
}

uint32_t kmalloc_ap(uint32_t sz, uint32_t *phys) {
    return kmalloc_imp(sz, 1, phys);
}

uint32_t kmalloc(uint32_t sz) {
    return kmalloc_imp(sz, 0, NULL);
}
