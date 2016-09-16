#include <stdint.h>
#include "kheap.h"

uint32_t kheap_start;
uint32_t kheap_end;
uint32_t kheap_next_pageentry_allocated = 0;

struct header {
    struct header *next;
    uint32_t size;
    uint8_t free;
};

void initialize_kheap(uint32_t start_addr)
{
    kheap_start = start_addr;
    kheap_end = start_addr + 0x1000;
}

void *alloc(uint32_t size) {
    
}
