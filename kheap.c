#include "kheap.h"

extern uint32_t end_of_kernel;
uint32_t placement_address = (uint32_t)&end_of_kernel;

static inline uint32_t kmalloc_int(uint32_t size, int align, uint32_t *phys) {
    if(align && (placement_address & 0xFFFFF000)) {
        placement_address &= 0xFFFFF000;
        placement_address += 0x1000;
    }
    if(phys) {
        *phys = placement_address;
    }
    uint32_t tmp = placement_address;
    placement_address += size;
    return tmp;
}

// Standard allocate
uint32_t kmalloc(uint32_t size){
    return kmalloc_int(size, 0, 0);
}

// Page Aligned
uint32_t kmalloc_a(uint32_t size){
    return kmalloc_int(size, 1, 0);
}

// Get Physical Address
uint32_t kmalloc_p(uint32_t size, uint32_t *phys){
    return kmalloc_int(size, 0, phys);
}

// Aligned with physical address
uint32_t kmalloc_ap(uint32_t size, uint32_t *phys){
    return kmalloc_int(size, 1, phys);
}
