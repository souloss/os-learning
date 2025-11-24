#ifndef ORDERED_ARRAY_H
#define ORDERED_ARRAY_H

#include "types.h"

// An comparator function is used to sort elements. It returns -1, 0 or 1 if
// the first element is less than, equal to or greater than the second.
typedef int32_t (*comparator_t)(type_t, type_t);

typedef struct ordered_array
{
    type_t *array;
    uint32_t size;
    uint32_t max_size;
    comparator_t comparator;
} ordered_array_t;

int32_t standard_comparator(type_t a, type_t b);

ordered_array_t ordered_array_create(type_t *array, uint32_t max_size, comparator_t comparator);

// Return 1 for success, or 0 for failure
uint32_t ordered_array_insert(ordered_array_t *this, type_t item);

// Return result pointer, or 0 if i is out of bound.
type_t ordered_array_get(ordered_array_t *this, uint32_t i);

uint32_t ordered_array_remove(ordered_array_t *this, uint32_t i);

uint32_t ordered_array_remove_element(ordered_array_t *this, type_t ele);

uint32_t ordered_array_find_element(ordered_array_t *this, type_t ele);

#endif
