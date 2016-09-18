#ifndef FRAME_H
#define FRAME_H

/**
 * Functions for allocating frames
 */

void init_frame_allocator(uint32_t system_frames);
void alloc_frame(struct page *page, int is_kernel, int is_writeable);
void free_frame(struct page *page);

#endif
