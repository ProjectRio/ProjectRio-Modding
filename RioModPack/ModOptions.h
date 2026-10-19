/*###########################################################
# ModOptions.h -- the mod toggle bytes
###########################################################*/
// Author: LittleCoaks
//
// One word per user-toggleable mod option, in claimed free memory.
// ModOptionOn(id) reads one; ModOptions_Reset() / ModOptions_ApplyDefaults()
// seed the block. Mods never include this; RioModPack.c gates them from
// outside. See docs/mod_options.md.

#ifndef MODOPTIONS_H
#define MODOPTIONS_H

#include "CGecko/Common.h"
#include "Include/types.h"

#define MODOPT_BASE 0x802EB010
#define MODOPT_MAX  16

/* Option ids. APPEND ONLY, and keep MODOPT_COUNT last. */
#define MODOPT_WIDESCREEN   0
#define MODOPT_CPU_SPRINT   1
#define MODOPT_INSTANT_RNG  2
#define MODOPT_DUPLICATES   3
#define MODOPT_SUPERSTARS   4
#define MODOPT_MUSIC        5
#define MODOPT_GECKO        6
#define MODOPT_NIGHT_MARIO  7
#define MODOPT_SWING_SKIP   8
#define MODOPT_COUNT        9

#define MODOPT_ADDR(id)    (MODOPT_BASE + (id) * 4)
#define ModOptionValue(id) VAR_ADDRESS(u32, MODOPT_ADDR(id))
#define ModOptionOn(id)    (ModOptionValue(id) != 0)

#define MODOPT_SENTINEL_IDX (MODOPT_MAX - 1)
#define MODOPT_SENTINEL     0x4F505431   /* 'OPT1' -- bump if defaults change */

static void ModOptions_ApplyDefaults(void)
{
    volatile u32* opt = (volatile u32*)MODOPT_BASE;

    if (opt[MODOPT_SENTINEL_IDX] == MODOPT_SENTINEL)
        return;

    opt[MODOPT_SENTINEL_IDX] = MODOPT_SENTINEL;
    opt[MODOPT_GECKO]        = 1;   /* extra gecko codes run unless turned off */
}

/* volatile on purpose: a plain zeroing loop becomes a memset call, and there is no libc. */
static void ModOptions_Reset(void)
{
    volatile u32* opt = (volatile u32*)MODOPT_BASE;
    int i;

    for (i = 0; i < MODOPT_MAX; i++)
        opt[i] = 0;

    ModOptions_ApplyDefaults();
}

#endif /* MODOPTIONS_H */
