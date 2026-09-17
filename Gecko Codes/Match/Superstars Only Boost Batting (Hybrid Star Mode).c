/*###########################################################
# Superstars Only Boost Batting (Hybrid Star Mode)
###########################################################*/
// Author: PeacockSlayer
// Which stat each store belongs to: docs/import_match_rules.md
#include "Include/types.h"

#define NOP 0x60000000

/* transferStatsToInMemRoster: the stores of the boosted non-batting stats */
static const u32 NON_BATTING_BOOST_STORES[] = {
    0x80042880,   /* Speed          */
    0x8004288C,   /* ThrowingArm    */
    0x800428A4,   /* CurveBallSpeed */
    0x800428B0,   /* FastBallSpeed  */
    0x800428BC,   /* cursedBall     */
    0x800428C8,   /* Curve          */
    0x800428D4,   /* curveControl   */
};

CGECKO(SuperstarsOnlyBoostBatting,
       .notes = "Superstar characters only get their batting boosted.\n"
                "Speed, pitching and throwing arm stay as normal.");
void SuperstarsOnlyBoostBatting(void)
{
    for (int i = 0; i < (int)LEN(NON_BATTING_BOOST_STORES); i++)
        PatchInstruction(NON_BATTING_BOOST_STORES[i], NOP);
}
