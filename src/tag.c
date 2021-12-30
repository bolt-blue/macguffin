#include "tag.h"

#include <stdlib.h>

internal u32 hash_djb2(const char *str);
internal u32 hash_to_bucket(u32 h, u32 sz);

/*
 * Return: Initialised TagHash struct
 */
struct TagHash taghash_init(uint32_t size)
{
    // TODO: Set errno on error
    struct TagHash new;
    new.base = calloc(size, sizeof(tagnode_t *));
    new.size = size;
    return new;
}

void taghash_free(struct TagHash *table)
{
    if (!table->base)
        return;

    for (u32 i = 0; i < table->size; i++) {
        tagnode_t *cur = table->base[i];
        while (cur) {
            tagnode_t *tmp = cur;
            cur = cur->next;
            free(tmp);
        }
    }

    free(table->base);
}

/*
 * Expects:
 * - A unique tag_text. Use `taghash_exists()` first, if necessary.
 *   Nothing is done here to prevent duplicate entries.
 * Return:
 * Tag id: Success
 * NOTAG : Failure
 * TODO: Use errno instead for error handling
 */
tagid_t taghash_insert(struct TagHash *table, char *tag_text)
{
    tagid_t tid = hash_djb2(tag_text);

    tagnode_t *node = malloc(sizeof(tagnode_t));
    if (!node)
        return NOTAG;

    node->tag.id = tid;
    node->tag.text = tag_text;

    u32 bucket = hash_to_bucket(tid, table->size);
    node->next = table->base[bucket];
    table->base[bucket] = node;

    return tid;
}

/*
 * Parameters:
 * t_out: If not null, written to if tid is found
 * Return: boolean
 * 1: Found
 * 0: Not found
 */
u8 taghash_find_id(const struct TagHash *table, const tagid_t tid, struct Tag *t_out)
{
    u32 bucket = hash_to_bucket(tid, table->size);
    tagnode_t *cur = table->base[bucket];
    while (cur) {
        if (cur->tag.id == tid) {
            if (t_out)
                *t_out = cur->tag;
            return 1;
        }
        cur = cur->next;
    }
    return 0;
}

/*
 * Wrapper around `taghash_find_id()`
 */
u8 taghash_find_text(const struct TagHash *table, const char *text, struct Tag *t_out)
{
    tagid_t tid = hash_djb2(text);
    return taghash_find_id(table, tid, t_out);
}

/*
 * TODO: Does this behave well working with 32-bit int?
 *       The source uses unsigned long (64-bit)
 * Expects: A nul-terminated string
 * Ref: http://www.cse.yorku.ca/~oz/hash.html
 */
u32 hash_djb2(const char *s)
{
    u32 hash = 5381;
    int c;

    while ((c = *s++))
        hash = ((hash << 5) + hash) + c;    // hash * 33 + c

    return hash;
}

u32 hash_to_bucket(u32 h, u32 sz)
{
    return h % sz;
}
