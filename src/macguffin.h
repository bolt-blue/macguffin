/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MACGUFFIN_H
#define MACGUFFIN_H

#include "dynarr.h"
#include "stack.h"

enum CHOICE {
    SEARCH = 0x1,
    BROWSE,
    DIRADD,
    QUIT
};

struct RootDir {
    char *path;
    struct DynArr videos;
};

struct Video {
    char *filepath;
    char *title;
    u16 year;                   // Full year e.g. 1984
    u16 duration;               // In seconds (max. 18.2 hours)
    struct DynArr tags;         // TODO: Do we store this here?
};

struct AppState {
    struct Stack strings;       // Raw strings; directory and file paths, etc
    struct DynArr tracked_dirs; // Tracked Directories and their Videos
};

int load_state(struct AppState *state);
int save_state(struct AppState *state);
int add_directory(struct AppState *state);

#endif /* MACGUFFIN_H */
