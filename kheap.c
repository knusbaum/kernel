#include <stdint.h>
#include "common.h"
#include "kheap.h"
#include "terminal.h"
#include "isr.h"
#include "paging.h"

#define INITIAL_HEAP_SIZE (0x1000 * INITIAL_HEAP_PAGE_COUNT)

struct header {
    uint8_t free;
    uint32_t size;
};

struct free_header {
    struct header h;
    struct free_header *next;
    struct free_header *prev;
};


struct footer {
    uint32_t size;
};

char *memhead;
char *memend; // memend MUST always be page aligned.

struct free_header *head;

uint32_t allocations = 0;

static void do_kfree(void *p);

void malloc_stats() {
//    printf("Heap starts @ %p and contains %d bytes.\n", memhead, memend - memhead);
    terminal_writestring("Heap starts @ ");
    terminal_write_hex((uint32_t)memhead);
    terminal_writestring(" ends @ ");
    terminal_write_hex((uint32_t)memend);
    terminal_writestring(" and contains ");
    terminal_write_dec(memend - memhead);
    terminal_writestring(" bytes.\nCurrent allocations: [");
    terminal_write_dec(allocations);
    terminal_writestring("]\nFree bytes: [");
    uint32_t free = 0;
    struct free_header *current = head;
    while(current) {
        free += current->h.size;
        current = current->next;
    }
    terminal_write_dec(free);
    terminal_writestring("]\n");
}

static struct header *set_header_footer(char *block, uint32_t block_size) {
    struct header *header = (struct header *)block;
//    printf("Writing header @ %p\n", block);
    header->free = 1;
    header->size = block_size;
    struct footer *foot = (struct footer *)(block + block_size - sizeof (struct footer));
//    if(foot == 0x08000FFC) {
//        terminal_writestring("Setting footer @ 0x08000FFC to: ");
//        terminal_write_dec(block_size);
//        terminal_putchar('\n');
//        struct page *p = get_kernel_page(0x08000FFC, 0);
//        terminal_writestring("Physical page: ");
//        terminal_write_hex(p->frame << 12);
//        terminal_putchar('\n');
//    }
    foot->size = block_size;
    if(foot->size != block_size) {
//        terminal_writestring("Block: ");
//        terminal_write_hex(block);
//        terminal_writestring("\nSetting footer @ ");
//        terminal_write_hex(&foot->size);
//        terminal_writestring(" to: ");
//        terminal_write_dec(block_size);
//        terminal_putchar('\n');
//        struct page *p = get_kernel_page(&foot->size, 0);
//        terminal_writestring("Physical page: ");
//        terminal_write_hex(p->frame << 12);
//        terminal_putchar('\n');
        PANIC("OOM!");
    }
//    printf("Writing size %d to footer @ %p\n", foot->size, foot);
    return header;
}

static void expand(uint32_t at_least) {
    //terminal_writestring("entered expand function.\n");
    uint32_t size = 0x2000;
    if(at_least > size) {
        size = (at_least & 0xFFFFF000) + 0x1000;
    }
    uint32_t num_frames = size / 0x1000;
    char *block_header = memend;
//    terminal_writestring("Mapping ");
//    terminal_write_dec(num_frames);
//    terminal_writestring(" frames into kernel memory for heap!\n");
    
    uint32_t i;
    uint32_t actually_allocated = 0;
    for(i = 0; i < num_frames; i++) {
        struct page *page = map_kernel_page((uint32_t)memend, 0);
        if(page == NULL) {
            //terminal_writestring("Failed to map a kernel page!\n");
//PANIC("Ran out of page table entries for kernel!");
            break;
        }
        memend += 0x1000;
        actually_allocated += 0x1000;
    }
    if(actually_allocated == 0) {
        return;
    }
    
    set_header_footer((char *)block_header, actually_allocated);
    if(actually_allocated < size) {
        //terminal_writestring("Failed to allocate all desired pages.\n");
    }
    else {
        //terminal_writestring("Allocated ");
        //terminal_write_dec(num_frames);
        //terminal_putchar(' ');
    }
//    if(block_header >= 0x08000000) {
//        terminal_writestring("Allocated ");
//        terminal_write_hex(block_header);
//        terminal_putchar('\n');
//        terminal_writestring("Footer: ");
//        terminal_write_hex(block_header + actually_allocated - sizeof (struct footer));
//        terminal_putchar('\n');
//    }
    do_kfree(block_header + sizeof (struct header));
}
//
 //static void initialize_malloc() {
////    printf("Initializing malloc!\n");
//    memhead = sbrk(INITIAL_HEAP_SIZE);
//    memend = memhead + INITIAL_HEAP_SIZE;
//    set_header_footer(memhead, INITIAL_HEAP_SIZE);
//    head = (struct free_header *)memhead;
//    head->next = NULL;
//    head->prev = NULL;
////    printf("Heap initialized @ %p\n", memhead);
//}

void initialize_kheap(uint32_t start_addr) {
    memhead = (char *)start_addr;
    memend = memhead + INITIAL_HEAP_SIZE;
    set_header_footer(memhead, INITIAL_HEAP_SIZE);
    head = (struct free_header *)memhead;
    head->next = NULL;
    head->prev = NULL;
}

static struct free_header *find_block_with_page_aligned_addr(uint32_t needed_size) {
    struct free_header *block = head;
    while(block != NULL) {
        char *start_addr = (char *)block;
        char *end_addr = (char *)(start_addr + block->h.size);

        uint32_t next_page = (((uint32_t)block) & 0xFFFFF000) + 0x1000;

        // Move the blockstart back by the size of the header. The
        // address we RETURN has to be page-aligned.
        char *blockstart = (char *)(next_page - sizeof (struct header));

        if(blockstart < (start_addr + block->h.size - sizeof (struct footer))) {
            // page-aligned address is within block bounds!
            // Check if there is enough space.
            uint32_t total_space = end_addr - blockstart;
            if(total_space >= needed_size) {
                // WE FOUND ONE!
                if(blockstart != start_addr) {
                    // Now we need to break up the blocks.
                    if((uint32_t)(blockstart - start_addr) < (uint32_t)(sizeof(struct header) + sizeof(struct footer))) {
                        // There's not enough room before blockstart to split the blocks.
                        // Try to find another.
                        continue;
                    }
                    else {
                        // There's enough room. Break up the block.
                        uint32_t space_in_prev_block = blockstart - start_addr;
                        set_header_footer((char *)block, space_in_prev_block);
                        struct free_header *newblock = (struct free_header *)blockstart;
                        set_header_footer((char *)newblock, total_space);
                        newblock->next = block->next;
                        if(newblock->next) {
                            newblock->next->prev = newblock;
                        }
                        block->next = newblock;
                        newblock->prev = block;
                        // We've broken up the block. Return the new one!
                        return newblock;
                    }
                }
                else {
                    // The block is already page-aligned. Just return it.
                    return  block;
                }
            }
        }
        block = block->next;
    }
    return NULL;
}

static struct free_header *find_block(uint32_t size, uint8_t align) {
    struct free_header *block = head;
    while(block != NULL) {
//        printf("Checking block @ %p\n", block);
        // If we're not looking for alignment, Just check size:
        if(!align) {
            if(block->h.size >= size) {
                break;
            }
        }
        else {
            // We need an aligned block.
            block = find_block_with_page_aligned_addr(size);
            break;
        }
        block = block->next;
    }
    return block;
}

void *kmalloc(uint32_t size, uint8_t align, uint32_t *phys) {
    if(size <= 0) {
        return NULL;
    }

    if(memhead == NULL) {
        PANIC("kheap not initialized.");
        //initialize_malloc();
    }

    // The block we need needs to fit user data + metadata.
    uint32_t needed_size = size + sizeof (struct free_header) + sizeof (struct footer);
//    printf("User asked for %d. Total needed: %d\n", size, needed_size);

//    printf("Finding free block.\n");
    struct free_header *block = find_block(needed_size, align);
//    while(block != NULL) {
////        printf("Checking block @ %p\n", block);
//        // If we're not looking for alignment, Just check size:
//        if(!align) {
//            if(block->h.size >= needed_size) {
//                break;
//            }
//        }
//        else {
//            // We need an aligned block.
//            block = find_block_with_page_aligned_addr(needed_size);
//            break;
//        }
//        block = block->next;
//
//    }
    if(block == NULL) {
        //PANIC("Failed to expand kheap.");
//        printf("No free blocks large enough. Expanding.\n");
        //terminal_writestring("Heap exhausted. Expanding... ");
        expand(needed_size);
        //terminal_writestring("Done!\n");
//        block = head;
//        while(block != NULL) {
////            printf("Checking block @ %p\n", block);
//            if(block->h.size >= needed_size) {
//                break;
//            }
//            block = block->next;
//        }
        block = find_block(needed_size, align);
        if(block == NULL) {
            PANIC("Failed to expand kheap.");
        }
    }

//    printf("Using block @ %p of size %d\n", block, block->h.size);

//    printf("Removing block from free list.\n");
    block->h.free = 0;

    if(head == block) {
        head = block->next;
    }

    if(block->prev) {
//        printf("Block->prev @ %p\n", &block->prev);
        block->prev->next = block->next;
    }
    if(block->next) {
        block->next->prev = block->prev;
    }


    if(block->h.size - needed_size < sizeof (struct free_header) + sizeof (struct footer)) {
//        printf("Remainder not large enough. Absorbing surrounding space.\n");
        // Not enough room left in the remaining block to put a free entry. Just include the
        // rest of the space in the alloc'd block.
        needed_size = block->h.size;
    }
    else {
        void *blocknext = block->next;
        void *blockprev = block->prev;
        // Thse may get clobbered when writing the remainder header.
        // Save them now so we can use them to insert the remainder.
        struct free_header *remainder_block = (struct free_header *)(((char *) block) + needed_size);
        set_header_footer((char *)remainder_block, block->h.size - needed_size);

        remainder_block->next = blocknext;
        if(remainder_block->next) {
            remainder_block->next->prev = remainder_block;
        }
        remainder_block->prev = blockprev;
        if(remainder_block->prev) {
            remainder_block->prev->next = remainder_block;
        }
        else {
            // Blockprev was head. Set head to remainder.
            head = remainder_block;
        }
//        printf("Chopped up block. Remainder @ %p of size %d added to free list.\n", remainder_block, remainder_block->h.size);
    }

    block->h.size = needed_size;
    struct footer *block_footer = (struct footer *)((char *)block + needed_size - sizeof (struct footer));
    block_footer->size = needed_size;
//    printf("(NEW BLOCK) Writing size %d to footer @ %p\n", block_footer->size, block_footer);
//    printf("Block footer @ %p\n", ((char *)block) + block->h.size - sizeof (struct footer));

//    printf("Returning block's data pointer @ %p\n", ((char *)block) + sizeof (struct header));
    allocations++;

    char *return_addr = ((char *)block) + sizeof (struct header);
    
    if(phys) {
        struct page *p = get_kernel_page((uint32_t)return_addr, 0);
        *phys = (p->frame << 12) | (((uint32_t)return_addr) & 0xFFF);
    }
    
    return return_addr;
}

static struct free_header *get_previous_block(struct header *block_head) {
    char *block = (char *)block_head;
    if(block == memhead) {
        // We're at the beginning of the heap.
        // No previous block!
        return NULL;
    }
    struct footer *prev_foot = (struct footer *)(block - sizeof (struct footer));
    struct header *prev_head = (struct header *)(block - prev_foot->size);
    if(prev_head == block_head) {
//        printf("Something is wrong with the header @ %p! size should not be 0!\n", prev_foot);
        //exit(1);
        malloc_stats();
        terminal_writestring("zero footer @ ");
        terminal_write_hex((uint32_t)prev_foot);
        terminal_putchar('\n');
        PANIC("Detected kheap corruption.");
    }
    return (struct free_header *)prev_head;
}

static struct free_header *get_next_block(struct header *block_head) {
    char *block = (char *)block_head;
    void *nextblock = block + block_head->size;
    if(nextblock == memend) {
        return NULL;
    }
    return nextblock;
}

static void do_kfree(void *p) {
    if(p == NULL) {
        return;
    }
    struct free_header *block_header = (struct free_header *)(((char *)p) - sizeof (struct header));
    block_header->h.free = 1;
//    printf("Freeing block at %p of size: %d\n", block_header, block_header->h.size);

    struct free_header *prev_block = get_previous_block((struct header *)block_header);
    struct free_header *next_block = get_next_block((struct header *)block_header);

//    printf("Checking if we can join with previous block: %p and next block %p.\n", prev_block, next_block);

    // Try to join the prev block first.
    if(prev_block && prev_block->h.free) {
//        printf("Joining with previous block @ %p\n", prev_block);
        // If the previous block is free, just add our size to it.
        set_header_footer((char *)prev_block, prev_block->h.size + block_header->h.size);

        // If the next block is also free, we can join it simply.
        if(next_block && next_block->h.free) {
//            printf("Joining with next block @ %p\n", next_block);
            prev_block->next = next_block->next;
            if(prev_block->next) {
                prev_block->next->prev = prev_block;
            }
            set_header_footer((char *)prev_block, prev_block->h.size + next_block->h.size);
        }
//        printf("New block @ %p of size %d\n", prev_block, prev_block->h.size);
        return;
    }

    //Try to join with the next block
    if(next_block && next_block->h.free) {
//        printf("Joining with next block @ %p\n", next_block);
        block_header->next = next_block->next;
        if(block_header->next) {
            block_header->next->prev = block_header;
        }
        block_header->prev = next_block->prev;
        if(block_header->prev) {
            block_header->prev->next = block_header;
        }

        set_header_footer((char *)block_header, block_header->h.size + next_block->h.size);
        if(head == next_block) {
            head = block_header;
            block_header->prev = NULL;
        }
//        printf("New block @ %p of size %d\n", block_header, block_header->h.size);
        return;
    }

//    printf("Blocks on both sides are used. Looking backwards for closest free block!\n");

//    printf("Block footer @ %p\n", ((char *)block_header) + block_header->h.size - sizeof (struct footer));

    // We couldn't get pointers from either the previous or the next block since neither were free.
    // We won't be merging with any other blocks. We just want to find the closest previous block
    // So we can insert the block into the free list.
    while(prev_block) {
//        printf("DChecking block @ %p\n", prev_block);
        if(prev_block->h.free) {
            break;
        }
        prev_block = get_previous_block((struct header *)prev_block);
    }

    if(prev_block == NULL) {
        // There are no previous free blocks. This is the first free block. Set it at the head.
//        printf("No free blocks before this one! Inserting at list head!\n");
        block_header->next = head;
        head = block_header;
        if(block_header->next) {
            block_header->next->prev = block_header;
        }
        block_header->prev = NULL;
    }
    else {
//        printf("Inserting after block @ %p\n", prev_block);
        block_header->next = prev_block->next;
        if(block_header->next) {
            block_header->next->prev = block_header;
        }
        block_header->prev = prev_block;
        prev_block->next = block_header;
    }
    set_header_footer((char *)block_header, block_header->h.size);

}

// Tries to unmap the last block in the heap.
// Returns 1 on success.
static int unmap_blocks() {
//    return 0;
    // Check the last block to see if it's 1. free, 2. big enough that we can
    // unmap it.
    struct footer *f = (struct footer *)(memend - sizeof (struct footer));
    if(f->size < 0x1000) {
        // The block has to be at least a page large, otherwise, we can't
        // unmap a page (duh)
        return 0;
    }
    
    struct header *h = (struct header *)(memend - f->size);
    if(!h->free) {
        // The block isn't free anyway. We can't unmap it.
        return 0;
    }

    char *page_addr = memend - 0x1000;
    if(page_addr < (char *)h
       || (uint32_t)(page_addr - (char *)h) < (sizeof (struct free_header) + sizeof (struct footer))) {
        // There's not enough room left if we free the page to make a free block.
        return 0;
    }

    // We can free a page!

    uint32_t newsize = h->size - 0x1000;
    memend -= 0x1000;
    set_header_footer((char *)h, newsize);
    unmap_kernel_page((uint32_t)memend);
    return 1;
}

void kfree(void *p) {
    do_kfree(p);
    allocations--;
    while(unmap_blocks()){}
}
