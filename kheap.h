#ifndef KHEAP_H
#define KHEAP_H

#define INITIAL_HEAP_PAGE_COUNT 4

/**
 * initialize the kernel heap.
 * start_addr must be a pointer to a mapped kernel page.
 */
void initialize_kheap(uint32_t start_addr);

/**
 * Allocates a contiguout region of memory 'size' bytes in size.
 * equivalent to kmalloc_ap(size, 0, NULL);
 */
void *kmalloc(uint32_t size);

void *krealloc(void *p, uint32_t size);

/**
 * Allocates a contiguous region of memory 'size' in size.
 * If page_align==1, it creates that block starting on a page boundary.
 */
void *kmalloc_ap(uint32_t size, uint8_t page_align, uint32_t *phys);

/**
 * Releases a block allocated with 'alloc'.
 */
void kfree(void *p);

/**
 * Write kheap stats to the terminal.
 */
void malloc_stats();

#endif
