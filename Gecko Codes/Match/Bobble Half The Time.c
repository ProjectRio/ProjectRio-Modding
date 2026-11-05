/*###########################################################
# Bobble Half The Time
###########################################################*/
// Author: Nuche17
// BobbleArray's layout and when calculateBobble rolls: docs/import_match_gameplay.md
#include "Include/game/UnknownHomes_Game.h"
#include "Include/Symbols/game.h"
#include "Include/Rio/PatchTable.h"

#define BOBBLE_PERCENT 50

/* [Toy Field][character class][kind]: kinds 0-2 are ground balls (centre,
   side/running, diving), 3-5 the same for air balls. Percent, x10 vs a 0-999 roll. */
#define BobbleArray ARRAY_3D_ADDRESS(u8, 2, 4, 6, BobbleArray_ADDR)
#define FIELD_TYPES       2
#define CHARACTER_CLASSES 4
#define CATCH_KINDS       6

#define NOP 0x60000000

/* calculateBobble doubles the pitcher's air-ball chance (`slwi r27, r27, 1`),
   which at 50% would be a bobble every time. */
static const RioPatch BOBBLE_PATCHES[] = {
    { 0x806658F8, 0x577B083C, NOP },
};

/* Per frame: game.rel .data and .text, rewritten after each reload. */
CGECKO(BobbleHalfTheTime, .state = MSSB_GAME,
       .notes = "Every catch the game rolls a bobble for (regular,\n"
                "running and diving, in the air and on the ground)\n"
                "is a 50% bobble chance, for every fielder.");
void BobbleHalfTheTime(void)
{
    int field, charClass, kind;
    for (field = 0; field < FIELD_TYPES; field++)
        for (charClass = 0; charClass < CHARACTER_CLASSES; charClass++)
            for (kind = 0; kind < CATCH_KINDS; kind++)
                BobbleArray[field][charClass][kind] = BOBBLE_PERCENT;

    RioPatch_Apply(BOBBLE_PATCHES, RIO_PATCH_COUNT(BOBBLE_PATCHES), CGECKO_ACTIVE);
}
