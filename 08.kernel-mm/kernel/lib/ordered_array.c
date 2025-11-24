#include "ordered_array.h"

// It is not very useful.
int32_t standard_comparator(type_t a, type_t b)
{
    return a < b ? -1 : (a == b ? 0 : 1);
}

ordered_array_t ordered_array_create(type_t *array, uint32_t max_size, comparator_t comparator)
{
    ordered_array_t to_ret;
    to_ret.array = array;
    to_ret.size = 0;
    to_ret.max_size = max_size;
    to_ret.comparator = comparator;
    return to_ret;
}

uint32_t ordered_array_insert(ordered_array_t *this, type_t item)
{
    // Check boundary
    if (this->size >= this->max_size)
    {
        return 0;
    }

    uint32_t iterator = 0;
    while (iterator < this->size && this->comparator(this->array[iterator], item) <= 0)
    {
        iterator++;
    }

    this->size++;
    for (uint32_t i = this->size - 1; i > iterator; i--)
    {
        this->array[i] = this->array[i - 1];
    }
    this->array[iterator] = item;

    return 1;
}

type_t ordered_array_get(ordered_array_t *this, uint32_t i)
{
    if (i >= this->size)
    {
        return 0;
    }

    return this->array[i];
}

uint32_t ordered_array_remove(ordered_array_t *this, uint32_t i)
{
    if (i >= this->size)
    {
        return 0;
    }

    while (i < this->size - 1)
    {
        this->array[i] = this->array[i + 1];
        i++;
    }
    this->size--;
    return 1;
}

uint32_t ordered_array_remove_element(ordered_array_t *this, type_t ele)
{
    uint32_t index = this->size;
    for (uint32_t i = 0; i < this->size; i++)
    {
        if (this->array[i] == ele)
        {
            index = i;
            break;
        }
    }
    if (index == this->size)
    {
        return 0;
    }

    while (index < this->size - 1)
    {
        this->array[index] = this->array[index + 1];
        index++;
    }
    this->size--;
    return 1;
}

uint32_t ordered_array_find_element(ordered_array_t *this, type_t ele)
{
    uint32_t index = this->size;
    for (uint32_t i = 0; i < this->size; i++)
    {
        if (this->array[i] == ele)
        {
            index = i;
            break;
        }
    }
    return index;
}
