/*###########################################################
# Dictionary Replaces Menu Music
###########################################################*/
// Author: LittleCoaks
// How the menu music works, the loader task, and the fade/volume/relocation traps: docs/menu_music.md

#include "Include/game/UnknownHomes_Game.h"
#include "Include/musyx/musyx.h"
#include "Include/menus/yd_step.h"
#include "Include/Unknown/File_0x800b0a14.h"
#include "Include/Symbols/dol.h"
#include "Include/Rio/MenuMusic.h"

// claimed RAM (see ClaimedFreeMemory.h)
#define g_dmmLoading VAR_ADDRESS(u32, 0x802EC288) // a loader task is in flight
#define g_dmmTicks  VAR_ADDRESS(u32, 0x802EC28C)  // watchdog while loading
#define DMM_OWNER            0x802EC290           // fake owner node for the task
#define g_dmmLoadDone VAR_ADDRESS(u16, 0x802EC2A0)  // = DMM_OWNER + 0x10
#define g_dmmSavedVol VAR_ADDRESS(u32, 0x802EC2A8)  // 0 = nothing saved, else 0x100 | original
#define g_dmmGone   VAR_ADDRESS(u32, 0x802EC2AC)  // consecutive frames fx 0 has been missing

#define LOADER_TASK_FN      ((void (*)(void))0x80021758)  // audioFileLoaderTask (no _ADDR in the decomp yet)
#define PUSH_DICT_GROUP_FN  ((void*)0x800627C4)  // pushSoundGroup(0, *(0x800EF81C))
#define DICT_AUDIO_FILE     4                    // 0x800EF508[4]
#define MENU_MUSIC_FX       484
#define DICT_MUSIC_FX       0                    // group 0's only entry; marks it loaded
#define DICT_MUSIC_LAYER    0x8021               // 0x8000 = "layer", id 0x21
#define STOCK_MENU_LAYER    0x8022

#define fxGroupCount  VAR_ADDRESS(u16, dataFXGroupNum_ADDR)
#define FX_GROUP_ARRAY dataFXGroups_ADDR          // { u16 groupId; u16 numFx; u32; u32 fxTable; }
#define g_menuMusicVol    VAR_ADDRESS(u8,  0x803CB888)  // menuMusicStartVolume (no _ADDR in the decomp yet)

#define DMM_LOAD_TIMEOUT 900                     // ~15s; give up rather than hang silent

// the audio-file table slot the loader writes file 4's buffer into
#define g_dmmDesc VAR_ADDRESS(u32, 0x800EF81C)
#define pushDictGroup ((void (*)(void))PUSH_DICT_GROUP_FN)

// a loaded audio file's first section always begins 0x20 past the header
#define DMM_DESC_HDR 0x20

// how long fx 0 has to stay missing before we believe it is gone
#define DMM_SETTLE_FRAMES 30

// What file 4's descriptor slot currently holds.
enum {
    DMM_DESC_NONE = 0,   // nothing loaded into this slot yet
    DMM_DESC_RAW,        // loaded, not yet relocated -- a task is mid-flight
    DMM_DESC_READY,      // loaded and relocated exactly once: safe to push
    DMM_DESC_BAD         // relocated more than once: pushing it would fault
};

// Self-gates on CGECKO_ACTIVE (a pack redefines it to "Dictionary is the
// selected track") rather than CGECKO_GATE_ADDR, so it can undo its edits.

// Screens this runs on; a pack that offers this track must add the screen the choice is made on.
#ifndef DMM_SCREENS
#define DMM_SCREENS(sc) ((sc) == 5)
#endif

// The FX table moves on every scene change: never cache the entry pointer.
static u8* dmmFindFxEntry(u16 fxId)
{
    u16 groups = fxGroupCount;
    u16 i, j;

    for (i = 0; i < groups; i++)
    {
        u8* g     = (u8*)(FX_GROUP_ARRAY + i * 12);
        u16 count = *(u16*)(g + 2);
        u8* table = *(u8**)(g + 8);

        if (table == 0 || count > 200)          // sanity: array is live game state
            continue;

        for (j = 0; j < count; j++)
        {
            u8* entry = table + j * 10;
            if (*(u16*)entry == fxId)
                return entry;
        }
    }
    return 0;
}

// Classify file 4's descriptor by its first word: 0x20 raw, d+0x20 relocated
// once, anything else relocated twice (the loader task is not idempotent).
static int dmmDescriptorState(void)
{
    u32 d = g_dmmDesc;
    u32 prj;

    if (d < 0x80000000 || d >= 0x81800000)
        return DMM_DESC_NONE;           // never loaded this session

    prj = *(u32*)d;
    if (prj == DMM_DESC_HDR)
        return DMM_DESC_RAW;
    if (prj == d + DMM_DESC_HDR)
        return DMM_DESC_READY;
    return DMM_DESC_BAD;
}

// Debounced "group 0 is gone": the loaded-group array reads empty for a few
// frames around a screen change.
static int dmmGroupReallyGone(void)
{
    if (dmmFindFxEntry(DICT_MUSIC_FX) != 0)
    {
        g_dmmGone = 0;
        return 0;
    }
    if (g_dmmGone < DMM_SETTLE_FRAMES)
    {
        g_dmmGone++;
        return 0;
    }
    return 1;
}

// Stop the current track and let the music routine start it again next frame.
// Never use the fade u8 at 0x803C671A for this: that path deregisters the updater.
static void dmmRestartMusic(void)
{
    MenuMusic_StopVoice();
    MenuMusic_Release();                        // 0 = "not playing" -> routine restarts it
}

// Pin the menu music volume to the entry's authored volume (the routine starts
// at the menu volume, 105, not the entry's 127). Something writes it back, so per frame.
static void dmmPinVolume(u8* fx)
{
    if (g_dmmSavedVol == 0)
        g_dmmSavedVol = 0x100 | g_menuMusicVol;  // remember the stock value once

    g_menuMusicVol = *(fx + 6);                  // +6 = the entry's authored volume
}

// Put everything back. The loaded sound group is deliberately left loaded.
static void dmmStandDown(void)
{
    u8* fx = dmmFindFxEntry(MENU_MUSIC_FX);

    if (fx != 0 && *(u16*)(fx + 2) == DICT_MUSIC_LAYER)
    {
        *(u16*)(fx + 2) = STOCK_MENU_LAYER;
        if (MenuMusic_IsPlaying())
            dmmRestartMusic();                  // swap back audibly, not on next scene
    }
    if (g_dmmSavedVol != 0)
    {
        g_menuMusicVol = (u8)(g_dmmSavedVol & 0xFF);
        g_dmmSavedVol  = 0;
    }
    g_dmmLoading = 0;
    g_dmmGone    = 0;
}

CGECKO(DictionaryReplacesMenuMusic, .state = MSSB_MENU,
       .notes = "Plays the Dictionary theme on the main menu\n"
                "instead of the usual menu music.");
void DictionaryReplacesMenuMusic()
{
    u16 sc = VAR_ADDRESS(menuControlStruct*, menuControlVariables_ADDR)->currentScreen;
    u8* fx;

    if (!(CGECKO_ACTIVE))
    {
        dmmStandDown();
        return;
    }

    // off our screens, leave everything alone except handing the volume back
    if (!DMM_SCREENS(sc))
    {
        if (g_dmmSavedVol != 0)
        {
            g_menuMusicVol = (u8)(g_dmmSavedVol & 0xFF);
            g_dmmSavedVol  = 0;
        }
        return;
    }

    // is the Dictionary's sound group still pushed? its fx 0 is the marker
    if (dmmGroupReallyGone())
    {
        int desc = dmmDescriptorState();

        // never leave fx 484 pointing at a layer that cannot resolve (start returns -1, menu goes silent)
        fx = dmmFindFxEntry(MENU_MUSIC_FX);
        if (fx != 0 && *(u16*)(fx + 2) == DICT_MUSIC_LAYER)
            *(u16*)(fx + 2) = STOCK_MENU_LAYER;

        // resident and relocated once: push it straight back (a second loader task would double-relocate and crash)
        if (desc == DMM_DESC_READY)
        {
            pushDictGroup();
            g_dmmLoading = 0;
            g_dmmGone    = 0;
            return;
        }

        // a load is in flight; let it finish
        if (desc == DMM_DESC_RAW)
            return;

        // already double-relocated; pushing would fault and reloading would not repair it
        if (desc == DMM_DESC_BAD)
        {
            g_dmmLoading = 0;
            return;
        }

        if (g_dmmLoading == 0)
        {
            // hand the game a loader task for the Dictionary's audio file set
            u8* node = (u8*)insertGraphicDrawingFunction(LOADER_TASK_FN, 1);
            if (node == 0)
                return;                         // pool full, try again next frame

            g_dmmLoadDone = 0;
            *(u32*)(node + 0x0C) = DMM_OWNER;
            *(u32*)(node + 0x14) = (u32)PUSH_DICT_GROUP_FN;
            *(node + 0x18)       = 0;           // state 0 = begin the load
            *(node + 0x19)       = DICT_AUDIO_FILE;

            g_dmmTicks   = 0;
            g_dmmLoading = 1;
        }
        else if (++g_dmmTicks > DMM_LOAD_TIMEOUT)
        {
            g_dmmTicks   = 0;                   // let a later menu visit retry
            g_dmmLoading = 0;
        }
        return;
    }

    // inside the settle window: do nothing until it resolves
    if (g_dmmGone != 0)
        return;

    // NB: do NOT clear g_dmmGone here, or the debounce never fires
    g_dmmLoading = 0;                           // loaded; the task has finished

    fx = dmmFindFxEntry(MENU_MUSIC_FX);
    if (fx == 0)
        return;

    // pin the volume before any restart
    dmmPinVolume(fx);

    if (*(u16*)(fx + 2) != DICT_MUSIC_LAYER)
    {
        // point a stock (or freshly reloaded) table at the Dictionary track; restart only if something is playing
        *(u16*)(fx + 2) = DICT_MUSIC_LAYER;

        if (MenuMusic_IsPlaying())
            dmmRestartMusic();
    }
}
