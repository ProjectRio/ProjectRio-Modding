/*###########################################################
# DictionaryReroute.h -- shared reroute + music fix for the Dictionary scene
###########################################################*/
// Author: LittleCoaks
//
// Call DictionaryReroute_Tick() once per frame from a per-frame code (a
// CGECKO() with no .address and .state = MSSB_MENU). The Records button then
// opens the Dictionary scene, and the menu music stops/resumes around it.
// See docs/custom_music.md.

#ifndef DICTIONARY_REROUTE_H
#define DICTIONARY_REROUTE_H
#include "Include/game/UnknownHomes_Game.h"
#include "Include/menus/yd_step.h"
#include "Include/musyx/musyx.h"

#define g_dictPrevScreen VAR_ADDRESS(u16, 0x802EC280)
#define g_dictResuming   VAR_ADDRESS(u32, 0x802EC284)

#define g_menuMusicHandle VAR_ADDRESS(u32, 0x803C6714)
#define g_menuMusicGuard  VAR_ADDRESS(unsigned char, 0x803C6718)

static inline void DictionaryReroute_Tick(void)
{
    *(volatile u32*)0x80641848 = 0x38600007;            // li r3, 7

    u16 sc   = *(u16*)(*(u32*)0x803CBBCC + 2);           // menuCtrl->screenCode
    u16 prev = g_dictPrevScreen;

    if (sc == 7 && prev != 7)
    {
        ((void(*)(unsigned char))0x800D15FC)(0);        // synthKillAllVoices(0)
        g_menuMusicHandle = 0xFFFFFFFF;
    }
    else if (sc != 7 && prev == 7)
    {
        g_dictResuming = 0x0D1C7;
    }

    if (sc == 5 && g_dictResuming == 0x0D1C7)
    {
        u32 handle = g_menuMusicHandle;
        if (handle == 0 || handle == 0xFFFFFFFF)
            g_menuMusicGuard = 0;                        // retry the start
        else
            g_dictResuming = 0;                          // playing -> done
    }

    g_dictPrevScreen = sc;
}

#endif
