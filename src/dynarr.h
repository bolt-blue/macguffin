/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef DYNARR_H
#define DYNARR_H

/*
 * Dynamic Array
 * (often called a vector, but that name is a bit misleading)
 * - For uniform-sized data
 */

#include "util.h"

struct DynArr {
    u8 *base;       // Start of the array
    u8 *cur;        // Next free position in the array
    u8 *top;        // End of the array
    u32 unit_size;  // Size in bytes of data to be stored
    u32 size;       // Current number of elements
};

struct DynArr dynarr_init(u32 unit_size, u32 capacity);
void dynarr_free(struct DynArr *arr);
int dynarr_add(struct DynArr *arr, void *data);
int dynarr_remove(struct DynArr *arr);
int dynarr_insert(struct DynArr *arr, void *data, u32 pos);
int dynarr_delete(struct DynArr *arr, u32 pos);
void * dynarr_at(struct DynArr *arr, u32 pos);

#endif /* DYNARR_H */
