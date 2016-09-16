#ifndef KHEAP_H
#define KHEAP_H

/**
 * initialize the kernel heap.
 * start_addr must be a pointer to a mapped kernel page.
 */
void initialize_kheap(uint32_t start_addr);
/**
 * Allocates a contiguous region of memory 'size' in size. 
 * If page_align==1, it creates that block starting on a page boundary.
 */
void *alloc(uint32_t size, uint8_t page_align); //, heap_t *heap);

/**
 * Releases a block allocated with 'alloc'.
 */
void free(void *p); 

#endif
