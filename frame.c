#include "stdint.h"
#include "stddef.h"
#include "isr.h"
#include "paging.h"
#include "frame.h"
#include "kmalloc_early.h"
#include "common.h"


// We're going to track free frames in a stack. (array)
uint32_t stack_count = 0;     // The current capacity of the stack
uint32_t *free_frames = NULL;
int32_t top_of_stack = -1;

// If the stack is empty, we allocate from the end of physical memory.
// Since we've not allocated any pages, end_of_mem begins at 0.
uint32_t end_of_mem = 0;

uint32_t allocated_frames = 0;
uint32_t total_frames;

void init_frame_allocator(uint32_t system_frames)
{
    total_frames = system_frames;
    if(free_frames != NULL)
    {
        // We've already initialized the frame allocator!
        return;
    }

    // Allocate a big enough stack to hold all the frames on the system.
    // Should be at most 4 MiB.
    free_frames = (uint32_t *)e_kmalloc(system_frames * sizeof (uint32_t));
    stack_count = system_frames;
}

void alloc_frame(struct page *page, int is_kernel, int is_writeable)
{
    if (page->frame != 0)
    {
        return; // Frame was already allocated, return straight away.
    }
    allocated_frames++;
    uint32_t idx;
    if(top_of_stack >= 0)
    {
        // There are free frames in the stack!
        idx = free_frames[top_of_stack];
        top_of_stack--; // POP
    }
    else
    {
        // Otherwise, there were no free frames on the stack.
        // Grab one from the end of memory.
        if(end_of_mem >= total_frames) {
            // There are no free frames in the frame stack
            // and we're at the limit of our physical memory.
            PANIC("Cannot alloc frame. Out of memory!");
        }
        idx = end_of_mem;
        end_of_mem++;
    }
    page->present = 1;                  // Mark it as present.
    page->rw      = (is_writeable)?1:0; // Should the page be writeable?
    page->user    = (is_kernel)?0:1;    // Should the page be user-mode?
    page->frame   = idx;
}

void free_frame(struct page *page)
{
    top_of_stack++; // Advance to the next spot
    // Put the frame into the stack.
    if(((uint16_t)top_of_stack) >= stack_count)
    {
        // This should never happen because we're allocating
        // the full stack during initialization.
        PANIC("Frame pool is full! Something weird happened!");
    }

    free_frames[top_of_stack] = page->frame;
    page->frame = 0x0;
    allocated_frames--;
}
