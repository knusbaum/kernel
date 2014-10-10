#include "ordered_array.h"

int8_t standard_lessthan(void *a, void *b)
{
    return (a<b)?1:0;
}

ordered_array_t create_ordered_array(uint32_t max_size, lessthan_t less_than)
{
//    ordered_array_t to_ret;
//    to_ret.array = (void*)kmalloc( max_size * sizeof(void *));
//    memset(to_ret.array, 0, max_size * sizeof(void *));
//    to_ret.size=0;
//    to_ret.max_size = max_size;
//    to_ret.less_than = less_than;
//    return to_ret;
    void * array = kmalloc( max_size * sizeof(void *));
    place_ordered_array(array, max_size, less_than);
}

ordered_array_t place_ordered_array(void * addr, uint32_t max_size, lessthan_t less_than)
{
    ordered_array_t to_ret;
    to_ret.array = (void **)addr;
    memset(to_ret.array, 0, max_size * sizeof(void *));
    to_ret.size = 0;
    to_ret.max_size = max_size;
    to_ret.less_than = less_than;
    return to_ret;
}

void destroy_ordered_array(ordered_array_t *array)
{
    // kfree(array->array);
}

void insert_ordered_array(void *item, ordered_array_t *array)
{
    ASSERT(array->less_than);
    uint32_t iterator = 0;
    while(iterator < array->size && array->less_than(array->array[iterator], item))
    {
        iterator++;
    }
    
    if(iterator == array->size)
    {
        array->array[array->size++] = item;
    }
    else
    {
        void * temp = array->array[iterator];
        array->array[iterator] = item;
        while(iterator < array->size)
        {
            iterator++;
            void * temp2 = array->array[iterator];
            array->array[iterator] = temp;
            temp = temp2;
        }
        array->size++;
    }

}

void * lookup_ordered_array(uint32_t i, ordered_array_t *array)
{
    ASSERT(i < array->size);
    return array->array[i];
}

void remove_ordered_array(uint32_t i, ordered_array_t *array)
{
    while(i < array->size)
    {
        array->array[i] = array->array[i+1];
        i++;
    }
    array->size--;
}
