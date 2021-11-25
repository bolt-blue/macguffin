#include "dynarr.h"

#include <stdlib.h>
#include <string.h>

// TODO: Handle alignment and padding
struct DynArr
dynarr_init(u32 unit_size, u32 capacity)
{
    struct DynArr arr;
    u32 total_size = unit_size * capacity;
    arr.base = malloc(total_size);
    arr.cur = arr.base;
    arr.top = arr.base + total_size;
    arr.unit_size = unit_size;
    arr.size = 0;
    return arr;
}

void
dynarr_free(struct DynArr *arr)
{
    if (!arr->base)
        return;
    free(arr->base);
    arr->cur = NULL;
    arr->top = NULL;
    arr->unit_size = 0;
    arr->size = 0;
}

/*
 * Add a new data item to the end of the array
 * Return:
 *  0: Success
 * -1: No space remaining
 */
int
dynarr_add(struct DynArr *arr, void *data)
{
    // TODO: Actually make this dynamically resize! :)
    ASSERT(arr->cur + arr->unit_size <= arr->top);
    if (arr->cur == arr->top)
        return -1;
    memcpy(arr->cur, data, arr->unit_size);
    arr->cur += arr->unit_size;
    arr->size++;
    return 0;
}

/*
 * Remove data item from the end of the array
 * Return:
 *  0: Success
 * -1: Array already empty
 */
int
dynarr_remove(struct DynArr *arr)
{
    ASSERT(arr->cur - arr->unit_size >= arr->base);
    if (arr->cur == arr->base)
        return -1;
    arr->cur -= arr->unit_size;
    arr->size--;
    return 0;
}

// TODO: Implement these
int dynarr_insert(struct DynArr *arr, void *data, u32 pos);
int dynarr_delete(struct DynArr *arr, u32 pos);

/*
 * Retrieve data item at a given position in the array
 * Return:
 * NULL: Position requested out of bounds
 */
void *
dynarr_at(struct DynArr *arr, u32 pos)
{
    ASSERT(arr->base + arr->unit_size * pos <= arr->top - arr->unit_size);
    if (pos < 0 || arr->size < pos)
        return NULL;
    return arr->base + arr->unit_size * pos;
}
