/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef TAG_H
#define TAG_H

#include "util.h"

typedef u32 tagid_t;
#define NOTAG UINT32_MAX

struct Tag {
    char *text;
    tagid_t id;                     // Unique persistent ID
};

typedef struct TagNode {
    struct Tag tag;
    struct TagNode *next;
} tagnode_t;

struct TagHash {
    tagnode_t **base;
    u32 size;
};

struct TagHash taghash_init(uint32_t size);
void taghash_free(struct TagHash *table);
u32 taghash_insert(struct TagHash *table, char *tag_text);
// TODO: Should these functions just return the text or id (respectively) instead?
u8 taghash_find_id(const struct TagHash *table, const tagid_t tid, struct Tag *t_out);
u8 taghash_find_text(const struct TagHash *table, const char *text, struct Tag *t_out);

#endif /* TAG_H */
