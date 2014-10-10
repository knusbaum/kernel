#ifndef FRAME_H
#define FRAME_H

#include "paging.h"

#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

extern uint32_t *frames;
extern uint32_t nframes;

void alloc_frame(page_t *page, int is_kernel, int is_writeable);

#endif
