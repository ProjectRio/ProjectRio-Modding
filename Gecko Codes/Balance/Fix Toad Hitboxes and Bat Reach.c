/*###########################################################
# Fix Toad Hitboxes and Bat Reach
###########################################################*/
// Author: Roeming
// Which Toad ends up matching which: docs/import_balance.md
#include "Include/game/UnknownHomes_Game.h"
#include "Include/Symbols/game.h"

typedef struct { s16 v[8]; } FielderHitboxConstsEntry;
#define FielderHitboxConsts ARRAY_1D_ADDRESS(FielderHitboxConstsEntry, 55, FielderHitboxConsts_ADDR)

#define STOCK_RED_TOAD_HITBOX_0 70

static const s16 TOAD_FIELDING_HITBOX[8] = { 120, 80, 50, 185, 80, 350, 90, 50 };

#define RED_TOAD_REACH_HORIZONTAL_NEAR 0xBF733333   /* -0.95f */
#define RED_TOAD_REACH_HORIZONTAL_FAR  0x3F0CCCCD   /*  0.55f */
#define RED_TOAD_REACH_VERTICAL_FRONT  0xBDCCCCCD   /* -0.10f */
#define RED_TOAD_REACH_VERTICAL_BACK   0x3FE66666   /*  1.80f */

CGECKO(FixToadHitboxesAndBatReach, .state = MSSB_GAME,
       .notes = "Makes all five Toads play the same: Red Toad gets the other Toads' larger fielding hitbox,\n"
                "and the other Toads get Red Toad's longer bat reach.");
void FixToadHitboxesAndBatReach(void)
{
    FielderHitboxConstsEntry* redToad = &FielderHitboxConsts[CHAR_ID_TOAD_RED];
    if (redToad->v[0] != STOCK_RED_TOAD_HITBOX_0)
        return;

    for (int i = 0; i < 8; i++)
        redToad->v[i] = TOAD_FIELDING_HITBOX[i];

    for (int id = CHAR_ID_TOAD_BLUE; id <= CHAR_ID_TOAD_PURPLE; id++)
    {
        u32* reach = (u32*)&BatterHitbox[id];
        reach[0] = RED_TOAD_REACH_HORIZONTAL_NEAR;
        reach[1] = RED_TOAD_REACH_HORIZONTAL_FAR;
        reach[2] = RED_TOAD_REACH_VERTICAL_FRONT;
        reach[3] = RED_TOAD_REACH_VERTICAL_BACK;
    }
}
