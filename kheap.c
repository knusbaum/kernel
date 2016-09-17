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

void malloc_stats() {
//    printf("Heap starts @ %p and contains %d bytes.\n", memhead, memend - memhead);
    terminal_writestring("Heap starts @ ");
    terminal_write_hex((uint32_t)memhead);
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
    foot->size = block_size;
//    printf("Writing size %d to footer @ %p\n", foot->size, foot);
    return header;
}

//static void expand(uint32_t at_least) {
//    uint32_t size = memend - memhead;
//    if(at_least > size) {
//        size = (at_least & 0xFFFFF000) + 0x1000;
//    }
////    printf("Expanding by %d bytes!\n", size);
//    char *block_header = sbrk(size);
//    memend = block_header + size;
//    set_header_footer((char *)block_header, size);
////    printf("Adding new block @ %p to free list!\n", block_header);
//    my_free(block_header + sizeof (struct header));
//}
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
                    if(blockstart - start_addr < sizeof(struct header) + sizeof(struct footer)) {
                        // There's not enough room before blockstart to split the blocks.
                        // Try to find another.
                        continue;
                    }
                    else {
                        // There's enough room. Break up the block.
                        uint32_t space_in_prev_block = blockstart - start_addr;
                        set_header_footer(block, space_in_prev_block);
                        struct free_header *newblock = (struct free_header *)blockstart;
                        set_header_footer(newblock, total_space);
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

void *kmalloc(uint32_t size, uint8_t align, uint32_t *phys) {
    if(size <= 0) {
        return NULL;
    }

    if(memhead == NULL) {
        PANIC("kheap not initialized.");
        //initialize_malloc();
    }

    // The block we need needs to fit user data + metadata.
    uint32_t needed_size = size + sizeof (struct header) + sizeof (struct footer);
//    printf("User asked for %d. Total needed: %d\n", size, needed_size);

//    printf("Finding free block.\n");
    struct free_header *block = head;
    while(block != NULL) {
//        printf("Checking block @ %p\n", block);
        // If we're not looking for alignment, Just check size:
        if(!align) {
            if(block->h.size >= needed_size) {
                break;
            }
        }
        else {
            // We need an aligned block.
            block = find_block_with_page_aligned_addr(needed_size);
            break;
        }
        block = block->next;

    }
    if(block == NULL) {
        PANIC("Failed to expand kheap.");
//        printf("No free blocks large enough. Expanding.\n");
        //expand(needed_size);
        block = head;
        while(block != NULL) {
//            printf("Checking block @ %p\n", block);
            if(block->h.size >= needed_size) {
                break;
            }
            block = block->next;
        }
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
    return ((char *)block) + sizeof (struct header);
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

void kfree(void *p) {
    if(p == NULL) {
        return;
    }
    allocations--;
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
