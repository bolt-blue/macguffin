/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MACGUFFIN_H
#define MACGUFFIN_H

#include "dynarr.h"
#include "stack.h"
#include "tag.h"

/* ========================================================================== */

enum CHOICE {
    SEARCH = 0x1,
    BROWSE,
    DIRADD,
    QUIT
};

/*
 * NOTE: This should almost certainly only be passed by pointer
 */
struct AppState {
    struct Stack scratch;       // Temporary data; volatile
    struct Stack strings;       // Raw strings; directory and file paths, etc
    struct DynArr tracked_dirs; // Array of struct RootDir
    struct TagHash tags;        // Hash table of struct Tag
};

struct RootDir {
    char *path;
    struct DynArr videos;       // Array of struct Video
};

struct Video {
    char *filepath;
    char *title;
    u16 year;                   // Full year e.g. 1984
    u16 duration;               // In seconds (max. 18.2 hours)
    struct DynArr tags;         // Array of struct Tag ID's only
};

#define DEFAULT_TAG_AMT 8

/* ========================================================================== */

/*
 * Core function prototypes
 * All of these must be provided by each platform-specific layer
 * WARNING: Subject to change during alpha
 */

int load_state(struct AppState *state);
int save_state(struct AppState *state);

int add_directory(struct AppState *state);
void browse(struct AppState *state);

void add_tag(tagid_t tid, struct Video *v);
void add_tags(struct AppState *state, struct Video *v);
void remove_tags(struct AppState *state, struct Video *v);
void create_tags(struct AppState *state, struct Video *v);

int process_dir(struct AppState *state, char *path);
char *push_dir_path(struct Stack *stack, char *parent, char *path);

#endif /* MACGUFFIN_H */
