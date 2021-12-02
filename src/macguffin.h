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

struct Video {
    char *filepath;
    char *title;
    u16 year;                   // Full year e.g. 1984
    u16 duration;               // In seconds (max. 18.2 hours)
    struct DynArr tags;         // TODO: Do we store this here?
};

struct AppState {
    struct Stack strings;       // Raw video file paths [could hold other ?]
    struct DynArr videos;       // Array of struct Video
};

#endif /* MACGUFFIN_H */
