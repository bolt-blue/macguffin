#include "stack.h"

#include <stdlib.h>
#include <string.h>

struct entry_tail {
    u32 size;
};

struct Stack
stack_init(u32 capacity)
{
    struct Stack s;
    s.base = malloc(capacity);
    s.size = 0;
    s.capacity = capacity;
    return s;
}

void
stack_free(struct Stack *s)
{
    free(s->base);
    s->base = NULL;
    s->size = 0;
    s->capacity = 0;
}

/*
 * Reserve space on the stack
 * size: in bytes
 */
void *
stack_push(struct Stack *stack, u32 size)
{
    // TODO: Account for alignment when placing entry_tail struct
    u32 total_size = size + sizeof(struct entry_tail);

#if 0
    if (stack->size + total_size > stack->capacity)
        return NULL;
#else
    ASSERT(stack->size + total_size <= stack->capacity);
#endif

    u8 *pushed = stack->base + stack->size;
    struct entry_tail *tail = (void *)(pushed + size);

    tail->size = size;
    stack->size += total_size;

    return pushed;
}

void *stack_pop(struct Stack *stack)
{
    if (!stack->size)
        return NULL;

    struct entry_tail *last_tail = (void *)(stack->base + stack->size - sizeof(struct entry_tail));
    stack->size -= last_tail->size + sizeof(struct entry_tail);

    return stack->base + stack->size;
}
