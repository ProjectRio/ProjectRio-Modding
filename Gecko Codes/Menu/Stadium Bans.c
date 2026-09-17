/*###########################################################
# Stadium Bans
###########################################################*/
// Author: LittleCoaks
#include "Include/static/UnknownHomes_Static.h"
#include "Include/Dolphin/pad.h"

// Stadium-select scene state (heap, menus.rel): the hovered stadium id
#define StadiumSelectCursor VAR_ADDRESS(u8, 0x80750C37)

// One UI record per stadium icon, 0xC0 apart; zeroing this byte hides the icon
#define STADIUM_ICON_BASE    0x803B6EF7
#define STADIUM_ICON_STRIDE  0xC0
#define StadiumIconVisible(id) VAR_ADDRESS(u8, STADIUM_ICON_BASE + (id) * STADIUM_ICON_STRIDE)

CGECKO(StadiumBans, .address = 0x8065074C, .state = MSSB_MENU,
       .instruction = "lwz r3, 0(r3)",
       .notes = "On the stadium select screen, press X to ban the highlighted stadium\n"
                "(its icon is removed).");
void StadiumBans(void)
{
    controllerInputStruct* pads = Static_Stats_Tables.controllerInputs;
    u16 pressed = pads[0].currentHeldInput | pads[1].newInput;

    if (!(pressed & PAD_BUTTON_X))
        return;

    StadiumIconVisible(StadiumSelectCursor) = 0;
}
