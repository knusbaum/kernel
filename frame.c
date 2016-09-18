#include <stdint.h>
#include <stddef.h>
#include "isr.h"
#include "paging.h"
#include "frame.h"
#include "kmalloc_early.h"
#include "common.h"

// Only going to have 65536 frames for now. (256 MiB RAM)

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
//    // We might as well use up a full page,
//    // so allocate 4096 bytes (2048 page indecies)
//    free_frames = (uint16_t*)e_kmalloc_a(0x1000);
//    stack_count = 0x1000 / sizeof (uint16_t);

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
        top_of_stack--; // That frame is no longer on the stack.
    }
    else
    {
        // Otherwise, there were no free frames on the stack.
        // Grab one from the end of memory.
        if(end_of_mem >= total_frames) {
            // There are no free frames in the frame stack
            // and we're at the limit of our 256 MiB.
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
        // The stack is full! Allocate more stack space!
        // TODO: We need to do this once the actual kernel heap
        // is implemented. If this happens before the kheap is
        // initialized, the kernel should panic.
        // For now, we just panic.
        PANIC("Frame pool is full!");
    }
//    terminal_writestring("Adding frame: ");
//    terminal_write_hex(page->frame << 12);
//    terminal_writestring(" to pool.\n");



    free_frames[top_of_stack] = (uint16_t)(page->frame & 0xFFFF); // only 65536 pages (for now)
    page->frame = 0x0;
    allocated_frames--;
}
