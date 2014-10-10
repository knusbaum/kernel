#include "kheap.h"
#include "kheap_placement.h"
#include "ordered_array.h"

static int32_t find_smallest_hole(uint32_t size, uint8_t page_align, heap_t *heap)
{
    uint32_t iterator = 0;
    while(iterator < heap->index.size)
    {
        header_t * header=(header_t*)lookup_ordered_array(iterator, &heap->index);
        if(page_align > 0) {
            uint32_t location = (uint32_t)header;
            int32_t offset = 0;
            if (((location + sizeof(header_t)) & 0xFFFFF000) != 0)
            {
                offset = 0x1000 - (location + sizeof(header_t))%0x1000;
            }
            int32_t hole_size = (int32_t)header->size - offset;
            if(hole_size >= (int32_t)size){
                break;
            }
        }
        else if(header->size >= size) {
            break;
        }
        iterator++;
    }
    if(iterator == heap->index.size)
    {
        return -1;
    }
    else
    {
        return iterator;
    }
}

static int8_t hole_size_less_than(void *a, void *b)
{
    return (((header_t*)a)->size < ((header_t*)b)->size)?1:0;
}

heap_t *create_heap(uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t readonly)
{
    heap_t *heap = (heap_t*)kmalloc(sizeof(heap_t));

    // All our assumptions are made on startAddress and endAddress being page-aligned.
    ASSERT(start%0x1000 == 0);
    ASSERT(end%0x1000 == 0);

    // Initialise the index.
    heap->index = place_ordered_array( (void*)start, HEAP_INDEX_SIZE, &hole_size_less_than);

    // Shift the start address forward to resemble where we can start putting data.
    start += sizeof(void *)*HEAP_INDEX_SIZE;

    // Make sure the start address is page-aligned.
    if ((start & 0xFFFFF000) != 0)
    {
        start &= 0xFFFFF000;
        start += 0x1000;
    }
    // Write the start, end and max addresses into the heap structure.
    heap->start_address = start;
    heap->end_address = end;
    heap->max_address = max;
    heap->supervisor = supervisor;
    heap->readonly = readonly;

    // We start off with one large hole in the index.
    header_t *hole = (header_t *)start;
    hole->size = end-start;
    hole->magic = HEAP_MAGIC;
    hole->is_hole = 1;
    insert_ordered_array((void*)hole, &heap->index);

    return heap;
}
