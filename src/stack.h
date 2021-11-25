/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef STACK_H
#define STACK_H

#include "util.h"

struct Stack {
    u8 *base;
    u32 size;
    u32 capacity;
};

struct Stack stack_init(u32 capacity);
void stack_free(struct Stack *s);
void *stack_push(struct Stack *stack, u32 size);
void *stack_pop(struct Stack *stack);

#endif /* STACK_H */
