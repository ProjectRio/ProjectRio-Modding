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
#include "Include/Symbols/dol.h"
#include "Include/Rio/MenuMusic.h"

#define g_dictPrevScreen VAR_ADDRESS(u16, 0x802EC280)
#define g_dictResuming   VAR_ADDRESS(u32, 0x802EC284)

static inline void DictionaryReroute_Tick(void)
{
    *(volatile u32*)0x80641848 = 0x38600007;            // li r3, 7

    u16 sc   = VAR_ADDRESS(menuControlStruct*, menuControlVariables_ADDR)->currentScreen;
    u16 prev = g_dictPrevScreen;

    if (sc == 7 && prev != 7)
    {
        synthKillAllVoices(0);
        menuMusic.handle = MENUMUSIC_NO_VOICE;
    }
    else if (sc != 7 && prev == 7)
    {
        g_dictResuming = 0x0D1C7;
    }

    if (sc == 5 && g_dictResuming == 0x0D1C7)
    {
        if (!MenuMusic_IsPlaying())
            menuMusic.playing = 0;                       // retry the start
        else
            g_dictResuming = 0;                          // playing -> done
    }

    g_dictPrevScreen = sc;
}

#endif
