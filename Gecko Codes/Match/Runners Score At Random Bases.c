/*###########################################################
# Runners Score At Random Bases
###########################################################*/
// Author: Roeming, LittleCoaks, PeacockSlayer, others
// How the random distance is built from the RNG word: docs/import_match_rules.md
#include "Include/game/UnknownHomes_Game.h"

/* game.rel .rodata lbl_3_rodata_13B8, stock 4.0f; not in the decomp headers yet */
#define basesToScore VAR_ADDRESS(f32, 0x807AE8B0)

#define RANDOM_MANTISSA_BITS 0x3FF
#define EXPONENT_BASE        0x7800
#define FLOAT_FIELD_SHIFT    15
#define RANGE_SCALE          32.0f

CGECKO(RunnersScoreAtRandomBases, .address = 0x8069C6E8, .state = MSSB_GAME,
       .instruction = "lbz r0, 0xAD(r31)",
       .notes = "Runners score at a random spot on the base paths.\n"
                "The spot changes every half inning.");
void RunnersScoreAtRandomBases(void)
{
    union { u32 bits; f32 value; } roll;

    roll.bits = (((u32)g_Ball.StaticRandomInt1 & RANDOM_MANTISSA_BITS) | EXPONENT_BASE) << FLOAT_FIELD_SHIFT;
    basesToScore = roll.value * RANGE_SCALE;
}
