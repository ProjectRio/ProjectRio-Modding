/*###########################################################
# Manual Fielder Select
###########################################################*/
// Author: PeacockSlayer, LittleCoaks
// The state bytes, the select flow and the r6 redirect that skips the game's own pick: docs/manual_fielder_select.md
#include "Include/game/UnknownHomes_Game.h"
#include "Include/Symbols/game.h"

/* see ClaimedFreeMemory.h */
#define MFS_PREV_INPUT_ADDR 0x802EBF96
#define MFS_STATE_ADDR      0x802EBF97
#define MFS_PREV_INPUT      VAR_ADDRESS(u8, MFS_PREV_INPUT_ADDR)
#define MFS_STATE           VAR_ADDRESS(u8, MFS_STATE_ADDR)

#define MFS_NONE   0
#define MFS_SELECT 1
#define MFS_UNDO   2

#define fielderLockout ARRAY_1D_ADDRESS(u8, 9, fielderLockout_ADDR)

#define DEAD_BALL_REASON_OFFSET 0x1BD1       /* the .instruction's displacement */
#define RESET_FRAME 4

#define NO_FIELDER -1

static void ResetSelection(void)
{
    MFS_PREV_INPUT = MFS_NONE;
    MFS_STATE      = MFS_NONE;
}

/* Point the re-run `lbz r0, 0x1BD1(r6)` at MFS_STATE, which is non-zero on every
   path that gets here, so the game takes its "fielder already chosen" exit. */
static void SkipGameFielderPick(void)
{
    WRITE_GAME_REG(6, MFS_STATE_ADDR - DEAD_BALL_REASON_OFFSET);
}

static float Abs(float f) { return f < 0 ? -f : f; }

static int ClosestFielderNotHolding(float x, float z)
{
    s16 holding = g_FieldingLogic.selectedFielder;
    int best = NO_FIELDER;
    float bestDist = 0;

    for (int i = 0; i < (int)LEN(g_Fielders); i++)
    {
        if (i == holding)
            continue;
        float dist = Abs(x - g_Fielders[i].pos.x) + Abs(z - g_Fielders[i].pos.z);
        if (best == NO_FIELDER || !(bestDist < dist))
        {
            best = i;
            bestDist = dist;
        }
    }
    return best;
}

static void AssignFielder(int fielder)
{
    for (int i = 0; i < (int)LEN(g_Fielders); i++)
        if (g_Fielders[i].autoMovementFunctionIndex == AUTO_MOVEMENT_GOING_TO_BALL)
            g_Fielders[i].autoMovementFunctionIndex = AUTO_MOVEMENT_TRACK_HIT_BALL_PHASE2_AI_TEAM;

    g_Fielders[fielder].autoMovementFunctionIndex = AUTO_MOVEMENT_GOING_TO_BALL;
    g_FieldingLogic.selectedFielder        = fielder;
    g_FieldingLogic.selectedFielder_stored = fielder;
}

CGECKO(ManualFielderSelect, .address = 0x80678F8C, .state = MSSB_GAME,
       .instruction = "lbz r0, 0x1BD1(r6)",
       .notes = "R: select the closest fielder without the ball (closest to the landing\n"
                "spot while the ball is in the air, closest to the ball once it is down).\n"
                "Z: undo the selection. Does not work during an airborne star swing.");
void ManualFielderSelect(void)
{
    u16 framesSinceHit = g_Ball.framesSinceHit;

    if (framesSinceHit == RESET_FRAME)
    {
        ResetSelection();
        return;
    }
    if (g_Strikes.outs < 3
        && g_Ball.ballState != BALL_STATE_HIT && g_Ball.ballState != BALL_STATE_LOOSE)
    {
        ResetSelection();
        return;
    }
    if (g_Batter.isStarSwing && g_Ball.AtBat_ContactResult == BALL_RESULT_TYPE_IN_AIR)
    {
        MFS_STATE = MFS_NONE;
        return;
    }

    u16 buttons = g_FieldingLogic.fielderInputs;
    u8 pressed = MFS_NONE;
    if (buttons & INPUT_TRIGGER_Z)
        pressed = MFS_STATE = MFS_UNDO;
    if (buttons & INPUT_TRIGGER_R)
        pressed = MFS_STATE = MFS_SELECT;

    bool held = (MFS_PREV_INPUT == pressed);
    MFS_PREV_INPUT = pressed;

    if (MFS_STATE == MFS_NONE)
        return;
    if (held)
    {
        SkipGameFielderPick();
        return;
    }
    if (MFS_STATE == MFS_UNDO)
    {
        MFS_STATE = MFS_NONE;
        return;
    }

    int fielder;
    if (g_Ball.AtBat_ContactResult == BALL_RESULT_TYPE_LANDED)
        fielder = ClosestFielderNotHolding(g_Ball.AtBat_Contact_BallPos.x,
                                           g_Ball.AtBat_Contact_BallPos.z);
    else
        fielder = ClosestFielderNotHolding(g_Ball.physicsSubstruct.ballLandingSpotOrHeldSpot.x,
                                           g_Ball.physicsSubstruct.ballLandingSpotOrHeldSpot.z);

    if (fielder == NO_FIELDER || framesSinceHit < fielderLockout[fielder])
    {
        MFS_STATE = MFS_NONE;
        return;
    }

    AssignFielder(fielder);
    SkipGameFielderPick();
}
