#include "frame.h"


uint32_t *frames;
uint32_t nframes;

extern uint32_t placement_address;

static void set_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr/0x1000; // Why divide? I think we should just shift, right?
    uint32_t index = INDEX_FROM_BIT(frame);
    uint32_t offset = OFFSET_FROM_BIT(frame);
    frames[index] |= (0x1 << offset);
}

static void clear_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr/0x1000; // Why divide? I think we should just shift, right?
    uint32_t index = INDEX_FROM_BIT(frame);
    uint32_t offset = OFFSET_FROM_BIT(frame);
    frames[index] &= ~(0x1 << offset);
}

static uint32_t test_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr/0x1000; // Why divide? I think we should just shift, right?
    uint32_t index = INDEX_FROM_BIT(frame);
    uint32_t offset = OFFSET_FROM_BIT(frame);
    return (frames[index] & (0x1 << offset));
}

static uint32_t first_frame() {
    uint32_t i, j;
    for(i = 0; i < INDEX_FROM_BIT(nframes); i++)
    {
        if( frames[i] != 0xFFFFFFFF) // Nothing free
        {
            for (j = 0; j < 32; j++) {
                uint32_t toTest = 0x1 << j;
                if( !(frames[i] & toTest) )
                {
                    return i * 4 * 8 + j;
                }
            }
        }
    }
    return (uint32_t)-1;
}

void alloc_frame(page_t *page, int is_kernel, int is_writeable) {
    if( page->frame != 0) {
        return;  // Frame is already allocated.
    }
    else
    {
        uint32_t index = first_frame();
        if (index == (uint32_t)-1)
        {
            panic("no free frames!");
        }

        set_frame(index*0x1000);
        page->present = 1;
        page->rw      = (is_writeable) ? 1:0;
        page->user    = (is_kernel) ? 0:1;
        page->frame   = index;
    }
}

void free_frame(page_t * page)
{
    uint32_t frame;
    if (! (frame=page->frame) )
    {
        return; // The page didn't have a frame.
    }
    else
    {
        clear_frame(frame);
        page->frame=0x0;
    }
}
