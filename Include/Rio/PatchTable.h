/*###########################################################
# PatchTable.h -- apply / revert a table of instruction patches
###########################################################*/
// Author: LittleCoaks

#ifndef RIO_PATCHTABLE_H
#define RIO_PATCHTABLE_H

#include "CGecko/Common.h"

typedef struct RioPatch
{
    u32 addr;
    u32 orig;
    u32 patched;
} RioPatch;

#define RIO_PATCH_COUNT(t) ((int)(sizeof(t) / sizeof((t)[0])))

// Writes each site only when it holds the value being replaced (so a
// reloaded REL is re-patched and a patched site is left alone).
// Returns how many sites were written.
static inline int RioPatch_Apply(const RioPatch* t, int n, bool on)
{
    int wrote = 0;
    int i;

    for (i = 0; i < n; i++)
    {
        volatile u32* site = (volatile u32*)t[i].addr;
        u32 from = on ? t[i].orig    : t[i].patched;
        u32 to   = on ? t[i].patched : t[i].orig;

        if (*site != from)
            continue;
        *site = to;
        asm volatile("dcbst 0,%0\n\tsync\n\ticbi 0,%0\n\tisync" :: "r"(site) : "memory");
        wrote++;
    }
    return wrote;
}

#endif
