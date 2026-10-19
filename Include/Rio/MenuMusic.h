/*###########################################################
# MenuMusic.h -- hold or release the game's main-menu music voice
###########################################################*/
// Author: LittleCoaks
//
// MenuMusic_Hold() silences sfx 484 and keeps the stock routine from
// restarting it; MenuMusic_Release() hands the routine back so it restarts
// next frame. Engine notes: docs/menu_music.md.

#ifndef MENUMUSIC_H
#define MENUMUSIC_H

#include "Include/Unknown/File_0x80062a94.h"
#include "Include/musyx/musyx.h"
#include "Include/Symbols/dol.h"

#define MENUMUSIC_NO_VOICE 0xFFFFFFFF

#define synthKillAllVoices FUNCTION_ADDRESS(void, synthKillAllVoices_ADDR, u8)

static inline bool MenuMusic_IsPlaying(void)
{
    u32 h = menuMusic.handle;
    return h != 0 && h != MENUMUSIC_NO_VOICE;
}

static inline void MenuMusic_StopVoice(void)
{
    if (MenuMusic_IsPlaying())
        sndFXKeyOff(menuMusic.handle);
    menuMusic.handle = 0;
}

static inline void MenuMusic_Hold(void)
{
    MenuMusic_StopVoice();
    menuMusic.playing = 1;
}

static inline void MenuMusic_Release(void)
{
    menuMusic.playing = 0;
    menuMusic.handle  = 0;
}

#endif
