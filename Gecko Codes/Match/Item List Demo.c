/*###########################################################
# Item List Demo
###########################################################*/
// Author: LittleCoaks
// Demo of the ScreenList.h helper; only the P1 input read is game-state.

#include "Include/game/UnknownHomes_Game.h"

// 4 visible rows + 1 for the selected row's cursor glyph (see ScreenList.h's LIST_CURSOR_WIDTH note)
#define TEXT_SLOTS 5
#include "Include/Rio/ScreenList.h"

#include "Include/static/UnknownHomes_Static.h"
#include "Include/text/text_channel.h"
// 5 items, 4 rows visible at once -- the 5th only reachable by scrolling.
static ScreenList s_demoList = { 5, 0, 0, 4 };
static u16 s_prevButtons = 0;

CGECKO(ItemListDemo, .state = MSSB_GAME,
       .notes = "Developer test code. Leave this off.\n"
                "Shows a scrollable test list on screen (D-pad up/down moves it).");
void ItemListDemo()
{
    ScreenTextTick();

    // edge-detect: only move on the frame a direction is newly pressed
    u16 held    = (u16)g_InputBuffer.pads[0].button;
    u16 pressed = held & (u16)~s_prevButtons;
    s_prevButtons = held;

    if (pressed & PAD_BUTTON_DOWN)
        ScreenList_MoveDown(&s_demoList);
    if (pressed & PAD_BUTTON_UP)
        ScreenList_MoveUp(&s_demoList);

    const char* items[] =
    {
        "Weight Bat", "Iron Glove", "Fast Cleats", "Lucky Charm", "Power Band",
    };
    ScreenList_Draw(&s_demoList, 60, 100, 24, TEXT_SMALL, items);
}
