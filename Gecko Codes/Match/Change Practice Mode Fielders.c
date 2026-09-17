/*###########################################################
# Change Practice Mode Fielders
###########################################################*/
// Author: LittleCoaks
// Where the practice fielder list lives: docs/import_menu.md
#include "Include/Symbols/game.h"
#include "Include/mssbTypes.h"

#define PRACTICE_FIELDER_COUNT 8
#define PracticeFielders ((u8*)(constantList_ADDR + 0xA))

static const u8 betterFielders[PRACTICE_FIELDER_COUNT] =
{
    CHAR_ID_PETEY, CHAR_ID_MAGIKOOPA_BLUE, CHAR_ID_YOSHI, CHAR_ID_MAGIKOOPA_BLUE,
    CHAR_ID_YOSHI, CHAR_ID_YOSHI, CHAR_ID_YOSHI, CHAR_ID_YOSHI,
};

CGECKO(ChangePracticeModeFielders, .state = MSSB_GAME,
       .notes = "Changes the default fielders in practice mode to better characters.");
void ChangePracticeModeFielders(void)
{
    for (int i = 0; i < PRACTICE_FIELDER_COUNT; i++)
        PracticeFielders[i] = betterFielders[i];
}
