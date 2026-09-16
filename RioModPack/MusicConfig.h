/*###########################################################
# MusicConfig.h -- the soundtrack slots, their tracks, and how to find a file
###########################################################*/
// Author: LittleCoaks
//
// Shared by Custom Music.c (plays) and Options Menu.c (configures). Sixteen
// slots of claimed RAM, each holding a track id: MusicSlotTrack(slot) reads
// one, MusicTrackStep() walks the selectable tracks, MusicTrackAvailable()
// says whether a track's file is on this disc. Everything is static: the
// pack is one translation unit. See docs/custom_music.md.

#ifndef MUSICCONFIG_H
#define MUSICCONFIG_H

#include "CGecko/Common.h"
#include "Include/types.h"
#include "Include/Rio/DiscFst.h"

// ---- tracks ---------------------------------------------------------------
// The order here IS the order the Options menu steps through; bump
// MUSICCFG_MAGIC on any renumbering.
#define MUSIC_DEFAULT        0
#define MUSIC_STOCK_FIRST    1
#define MUSIC_STOCK_COUNT   15
#define MUSIC_STAR_FIRST    16
#define MUSIC_STAR_COUNT     2
#define MUSIC_LETTERS       18
#define MUSIC_DICTIONARY    19
#define MUSIC_CUSTOM_FIRST  20
#define MUSIC_CUSTOM_COUNT  10
#define MUSIC_OFF           30
#define MUSIC_TRACK_COUNT   31

#define MUSIC_TRACK_STREAM(t) ((t) - MUSIC_STOCK_FIRST)   // valid for stock ids
#define MUSIC_IS_STOCK(t)  ((t) >= MUSIC_STOCK_FIRST  && (t) < MUSIC_STOCK_FIRST + MUSIC_STOCK_COUNT)
#define MUSIC_IS_STAR(t)   ((t) >= MUSIC_STAR_FIRST   && (t) < MUSIC_STAR_FIRST + MUSIC_STAR_COUNT)
#define MUSIC_IS_CUSTOM(t) ((t) >= MUSIC_CUSTOM_FIRST && (t) < MUSIC_CUSTOM_FIRST + MUSIC_CUSTOM_COUNT)

// ---- slots ----------------------------------------------------------------
#define MUSIC_SLOT_MENU    0
#define MUSIC_SLOT_COUNT   16

// ---- claimed RAM (see ClaimedFreeMemory.h) --------------------------------
#define MUSICCFG_BASE      0x802EB540    // 16 words, one per slot
#define MUSIC_PATHBUF_BASE 0x802EB610    // 15 x 32 bytes, one per stream slot
#define MUSIC_PATHBUF_SIZE 32
#define MUSIC_SAVED_BASE   0x802EB590    // 15 x 8 bytes: each stream's stock {path,size}
#define MUSICCFG_MAGIC_ADDR 0x802EB580   // one-shot init sentinel
#define MUSICCFG_MAGIC     0x4D555334    // 'MUS4'

#define MusicSlot(i) (*(volatile u32*)(MUSICCFG_BASE + (i) * 4))

static const u8 s_musicSlotStream[MUSIC_SLOT_COUNT] =
{
    0xFF,   /* menu             */
    0,      /* Mario Stadium    */
    1,      /* Bowser Castle    */
    2,      /* Wario Palace     */
    3,      /* Yoshi Park       */
    4,      /* Peach Garden     */
    5,      /* DK Jungle        */
    6,      /* Replay           */
    7,      /* Results          */
    8,      /* Victory          */
    9,      /* Toy Field        */
    10,     /* Challenge Map    */
    11,     /* Demo             */
    12,     /* Ending Jingle    */
    13,     /* Staff Roll       */
    14,     /* Home Run Jingle  */
};

static const char* const s_musicSlotLabel[MUSIC_SLOT_COUNT] =
{
    "Menu",
    "Mario Stadium", "Bowser Castle", "Wario Palace", "Yoshi Park",
    "Peach Garden",  "DK Jungle",     "Replay",       "Results",
    "Victory",       "Toy Field",     "Challenge Map", "Demo",
    "Ending Jingle", "Staff Roll",    "Home Run Jing",
};

// Kept to 15 glyphs -- the Options menu draws these in a fixed column.
static const char* const s_musicTrackLabel[MUSIC_TRACK_COUNT] =
{
    "Default",
    "Mario Stadium", "Bowser Castle", "Wario Palace", "Yoshi Park",
    "Peach Garden",  "DK Jungle",     "Replay",       "Results",
    "Victory",       "Toy Field",     "Challenge Map", "Demo",
    "Ending Jingle", "Staff Roll",    "Home Run Jing",
    "Star 01", "Star 03", "Letters", "Dictionary",
    "Custom 01", "Custom 02", "Custom 03", "Custom 04", "Custom 05",
    "Custom 06", "Custom 07", "Custom 08", "Custom 09", "Custom 10",
    "Off",
};

// ---- disc lookup ----------------------------------------------------------
#define MUSIC_STREAM_TABLE  0x800E87B4                          // streamDescriptors (unbound extern)
#define MUSIC_STREAM_COUNT  15

static void MusicBuildPath(u32 track, char* out)
{
    const char* src;
    int i;

    if (MUSIC_IS_CUSTOM(track))
        src = "snd/my_snd_h/custom_00_h.adp";
    else if (track == MUSIC_OFF)
        src = "snd/my_snd_h/off_h.adp";          /* deliberately absent */
    else if (track == MUSIC_STAR_FIRST)
        src = "snd/my_snd_h/star_01_h.adp";
    else
        src = "snd/my_snd_h/star_03_h.adp";

    for (i = 0; i < MUSIC_PATHBUF_SIZE - 1 && src[i] != 0; i++)
        out[i] = src[i];
    out[i] = 0;

    if (MUSIC_IS_CUSTOM(track))
    {
        u32 n = track - MUSIC_CUSTOM_FIRST + 1;      /* 1..10 */
        out[20] = (char)('0' + (n / 10));
        out[21] = (char)('0' + (n % 10));
    }
}

static u32 MusicTrackAvailable(u32 slot, u32 track, char* scratch)
{
    if (track == MUSIC_DEFAULT || track == MUSIC_OFF || MUSIC_IS_STOCK(track))
        return 1;
    if (track == MUSIC_DICTIONARY || track == MUSIC_LETTERS)
        return slot == MUSIC_SLOT_MENU;
    if (!MUSIC_IS_STAR(track) && !MUSIC_IS_CUSTOM(track))
        return 0;                                    /* out of range */
    MusicBuildPath(track, scratch);
    return Fst_ProbePath(scratch) > 0;
}

/* Next selectable track in `dir`, wrapping; skips tracks whose file is absent. */
static u32 MusicTrackStep(u32 slot, u32 track, int dir, char* scratch)
{
    u32 t = (track < MUSIC_TRACK_COUNT) ? track : MUSIC_DEFAULT;
    int i;

    for (i = 0; i < MUSIC_TRACK_COUNT; i++)
    {
        if (dir > 0)
            t = (t + 1 < MUSIC_TRACK_COUNT) ? t + 1 : 0;
        else
            t = (t > 0) ? t - 1 : MUSIC_TRACK_COUNT - 1;

        if (MusicTrackAvailable(slot, t, scratch))
            return t;
    }
    return MUSIC_DEFAULT;
}

/* A slot's configured track, clamped: the config is plain RAM. */
static u32 MusicSlotTrack(u32 slot)
{
    u32 t = MusicSlot(slot);
    return (t < MUSIC_TRACK_COUNT) ? t : MUSIC_DEFAULT;
}

static void MusicConfig_Reset(void)
{
    volatile u32* cfg = (volatile u32*)MUSICCFG_BASE;
    int i;

    for (i = 0; i < MUSIC_SLOT_COUNT; i++)
        cfg[i] = MUSIC_DEFAULT;
}

#endif /* MUSICCONFIG_H */
