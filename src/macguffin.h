/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MACGUFFIN_H
#define MACGUFFIN_H

#include "dynarr.h"
#include "stack.h"
#include "tag.h"

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

int load_state(struct AppState *state);
int save_state(struct AppState *state);
int add_directory(struct AppState *state);

#endif /* MACGUFFIN_H */
