/*###########################################################
# Dictionary Scene
###########################################################*/
// Author: LittleCoaks
// The custom version of "Dictionary Replaces Records": same reroute + music
// fix, plus elements drawn on top of the stock scene. See docs/screen_dispatch.md.

#include "Include/Rio/DictionaryReroute.h"

// no payload statics (cgecko PIC hazard): all state lives in claimed RAM
#define SCREENTEXT_NO_FLOAT
#define TEXT_SLOTS 4
#define TEXT_BUFFER_ADDR 0x802EC0E8   // glyph scratch, see ClaimedFreeMemory.h
#include "Include/Rio/ScreenText.h"

#include "Include/static/UnknownHomes_Static.h"
#include "Include/text/text_channel.h"
#define g_magic  VAR_ADDRESS(u32, 0x802EC270)   // one-shot init sentinel
#define g_toggle VAR_ADDRESS(u32, 0x802EC274)   // our toggle "button" state

// reroute + music fix, every frame in menu state
CGECKO(DictionarySceneCommon, .state = MSSB_MENU,
       .notes = "Makes the Records button on the main menu open the game's unused Dictionary\n"
                "scene, with extra Project Rio elements drawn on top.\n"
                "Turn on this or \"Dictionary Replaces Records\", not both.");
void DictionarySceneCommon()
{
    DictionaryReroute_Tick();
}

// C2 at the stock Dictionary handler; .instruction re-runs its first
// instruction so the stock scene carries on after our overlay.
CGECKO(DictionaryScene, .address = 0x80693C44, .state = MSSB_MENU,
                        .instruction = "stwu r1, -48(r1)");
void DictionaryScene()
{
    if (g_magic != 0x0D1C7)          // init the toggle once (claimed RAM is whatever was there at power-on)
    {
        g_magic  = 0x0D1C7;
        g_toggle = 0;
    }

    // Y is free (the stock scene uses A/B/dpad); newInput is edge-detected by the menu
    controllerInputStruct* in = Static_Stats_Tables.controllerInputs;
    if (in[0].newInput & INPUT_BUTTON_Y)
        g_toggle = !g_toggle;

    // positions are low on screen to avoid the stock layout
    ScreenTextTick();
    WriteTextEx(320, 300, TEXT_WHITE, TEXT_LARGE, TEXT_CENTER, "Project Rio");
    WriteTextEx(320, 352, g_toggle ? TEXT_YELLOW : TEXT_GRAY, TEXT_SMALL, TEXT_CENTER,
                g_toggle ? "[*] Toggle: ON   (Y)"
                         : "[ ] Toggle: OFF  (Y)");
}
