#ifndef ORDERED_ARRAY_H
#define ORDERED_ARRAY_H

#include <stdint.h>
#include "common.h"

/**
 * comparator for ordering the list
 */
typedef int8_t (*lessthan_t)(void *, void *);

typedef struct
{
    void **array;
    uint32_t size;
    uint32_t max_size;
    lessthan_t less_than;
} ordered_array_t;

/**
 * Standard less-than
 */
int8_t standard_lessthan(void *a, void *b);

/**
 * Create an ordered array
 */
ordered_array_t create_ordered_array(uint32_t max_size, lessthan_t less_than);
ordered_array_t place_ordered_array(void * addr, uint32_t max_size, lessthan_t less_than);

/**
 * Destroy an ordered array
 */
void destroy_ordered_array(ordered_array_t *array);

/**
 * Add an item to the array
 */
void insert_ordered_array(void *item, ordered_array_t *array);

/**
 * Lookup item by index
 */
void * lookup_ordered_array(uint32_t i, ordered_array_t *array);

/**
 * Delete an item by index
 */
void remove_ordered_array(uint32_t i, ordered_array_t *array);


#endif
