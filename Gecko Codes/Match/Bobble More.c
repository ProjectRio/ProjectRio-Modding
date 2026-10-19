/*###########################################################
# Bobble More
###########################################################*/
// Author: taukhan, PeacockSlayer, LittleCoaks
// The three "no bobble" stores this overrides, and the hook sites: docs/import_match_gameplay.md
#include "Include/game/UnknownHomes_Game.h"
#include "Include/game/math/game_math.h"

#define BOBBLE_NONE 0
#define BOBBLE_AIR  3
#define NOP         0x60000000

#define STB_R3_BOBBLE_R30 0x987E0258
#define STB_R0_BOBBLE_R29 0x981D0258

static inline u8 RollBobble(void)
{
    return RandomInt_Game(2) ? BOBBLE_AIR : BOBBLE_NONE;
}

CGECKO(BobbleMore_CatchAction, .address = 0x8066B9BC, .state = MSSB_GAME,
       .instruction = "lbz r0, 0x1BC9(r5)",
       .notes = "Fielders bobble the ball half of the time.\n"
                "Careful when throwing to your teammate!");
void BobbleMore_CatchAction(void)
{
    READ_GAME_REG(InMemFielder*, fielder, 30);
    PatchInstruction_Conditional(0x8066B9C4, STB_R3_BOBBLE_R30, NOP);
    fielder->bobble = RollBobble();
}

CGECKO(BobbleMore_Reset, .address = 0x80665718, .state = MSSB_GAME,
       .instruction = "mr r31, r28");
void BobbleMore_Reset(void)
{
    READ_GAME_REG(int, fielderIndex, 28);
    PatchInstruction_Conditional(0x80665734, STB_R0_BOBBLE_R29, NOP);
    g_Fielders[fielderIndex].bobble = RollBobble();
}

CGECKO(BobbleMore_GroundBall, .address = 0x80665AE8, .state = MSSB_GAME);
void BobbleMore_GroundBall(void)
{
    READ_GAME_REG(InMemFielder*, fielder, 29);
    fielder->bobble = RollBobble();
}
