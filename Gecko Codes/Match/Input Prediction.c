/*###########################################################
# Input Prediction
###########################################################*/
// Author: LittleCoaks
// Batting half: docs/match_codes.md (Batter Lag Reduction). Pitching half and the differences: docs/import_match_rules.md
#include "Include/game/UnknownHomes_Game.h"
#include "Include/game/pitching/pitcher.h"

/* Rio-claimed words, see ClaimedFreeMemory.h. The ASM bodies reach them as
   lis 0x802E / ori, so the low halves are spelled out for them. */
#define IP_PREV_CURVE_INPUT  VAR_ADDRESS(u8,  0x802EBF98)
#define IP_CURVE_VELOCITY    VAR_ADDRESS(f32, 0x802EBFB4)
#define IP_PREV_X_CHANGE_LO  "0xBFA0"
#define IP_PREV_Y_CHANGE_LO  "0xBFA4"

#define CMPWI_R0_1 0x2C000001
#define CMPWI_R0_2 0x2C000002

/* game.rel bss byte the original code bumped alongside the swing frame; not in the decomp yet */
#define lbl_3_common_bss_32220_3 VAR_ADDRESS(u8, 0x80893303)

/* The synced header types g_Controls as a pointer; the game keeps four InputStructs here. */
#define g_ControlsByPort ARRAY_1D_ADDRESS(InputStruct, 4, 0x80893928)

static inline u8 CurveInput(u16 buttons)
{
    if (buttons & INPUT_BUTTON_LEFT)
        return INPUT_BUTTON_LEFT;
    return buttons & INPUT_BUTTON_RIGHT;
}

/*-----------------------------------------------------------
 Pitching: a changed curve input is applied twice on the frame it arrives
-----------------------------------------------------------*/

/* atBatScreen epilogue: remember the curve input this frame's pitch logic saw. */
CGECKO(InputPrediction_StoreCurveInput, .address = 0x8069E1D4, .state = MSSB_GAME,
       .instruction = "lwz r0, 0x14(r1)",
       .notes = "Predicts your inputs while pitching, batting and\n"
                "moving in the batter's box, to hide input delay.\n"
                "For laggy netplay or a slow TV.\n"
                "Not for use with Auto Golf Mode.");
void InputPrediction_StoreCurveInput(void)
{
    IP_PREV_CURVE_INPUT = CurveInput(g_FieldingLogic.fielderInputs);
}

/* Right after the pitch update's pitchCurve() call. */
CGECKO(InputPrediction_PitchCurve, .address = 0x806B05C4, .state = MSSB_GAME,
       .instruction = "lis r3, -0x7F77");
void InputPrediction_PitchCurve(void)
{
    u8 port = (u8)(g_GameLogic.teams[0] >> 24);

    IP_CURVE_VELOCITY = g_Pitcher.pitchCurveVeloV1;
    if (CurveInput(g_ControlsByPort[port].buttonInput) != IP_PREV_CURVE_INPUT)
    {
        pitchCurve();
        IP_CURVE_VELOCITY += g_Pitcher.pitchCurveVeloV1;
    }
    g_Pitcher.pitchCurveVeloV1 = IP_CURVE_VELOCITY;
}

/*-----------------------------------------------------------
 Batting: one frame of swing delay removed
-----------------------------------------------------------*/

CGECKO(InputPrediction_SwingPatches, .state = MSSB_GAME);
void InputPrediction_SwingPatches(void)
{
    if (VAR_ADDRESS(u32, &g_Batter.batPosition.y) == 0)
        return;

    hittableFrameInd[0][1] = 1;
    PatchInstruction_Conditional(0x806A3688, CMPWI_R0_1, CMPWI_R0_2);   /* batterAnimations */
    PatchInstruction_Conditional(0x80654610, CMPWI_R0_1, CMPWI_R0_2);   /* assignFrameCountersToAPointer */
}

/* calculateIfHitBall entry: a swing on its first frame is judged as frame 2. */
CGECKO(InputPrediction_ContactFrame, .address = 0x80651D48, .state = MSSB_GAME,
       .instruction = "lis r3, -0x7F77");
void InputPrediction_ContactFrame(void)
{
    if (g_Batter.framesSinceStartOfSwing != 1)
        return;

    g_Batter.framesSinceStartOfSwing = 2;
    g_Batter.Stored_Frame_SwingContact_SinceMiss = 2;
    lbl_3_common_bss_32220_3 = 2;
}

/* ifSwing: frameSwung takes the pitch hangtime counter one frame early.
   ASM because the value has to come back in r0; r5 is reloaded right after. */
ASM(InputPrediction_SwingFrame,
    "lha   0, 0x1B68(3)   \n"      /* the instruction we replace: g_Ball.pitchHangtimeCounter */
    "li    5, -1          \n"
    "cmpwi 0, 0xFFF       \n"
    "bgt   1f             \n"
    "add   0, 0, 5        \n"
    "1:                   \n"
    "nop                  \n",
    .address = 0x80652360, .state = MSSB_GAME);

/*-----------------------------------------------------------
 Batter's box movement (batterInBoxMovement, one hook per axis)
-----------------------------------------------------------*/

/* f0 = position, f2 = this frame's change; f0 must leave holding the result,
   so this stays ASM. Base points at the axis' previous-change word, +0xC its
   previous position, RESET_OFF the word the change resets from. */
#define BOX_MOVE_PREDICTION(BASE_LO, RESET_OFF)                              \
    "lis   8, 0x802E              \n"                                        \
    "ori   8, 8, " BASE_LO "      \n"                                        \
    "lfs   12, 0xC(8)             \n"                                        \
    "fcmpu 0, 0, 12               \n"                                        \
    "beq   1f                     \n"                                        \
    "lfs   12, " RESET_OFF "(8)   \n"                                        \
    "stfs  12, 0(8)               \n"                                        \
    "1:                           \n"                                        \
    "lfs   12, 0(8)               \n"                                        \
    "stfs  2, 0(8)                \n"                                        \
    "lis   8, 0x4000              \n"                                        \
    "stw   8, -8(1)               \n"                                        \
    "lfs   13, -8(1)              \n"                                        \
    "fmsub 2, 2, 13, 12           \n"                                        \
    "fadds 0, 0, 2                \n"                                        \
    "lis   8, 0x802E              \n"                                        \
    "ori   8, 8, " BASE_LO "      \n"                                        \
    "stfs  0, 0xC(8)              \n"

ASM(InputPrediction_BoxMoveX,
    BOX_MOVE_PREDICTION(IP_PREV_X_CHANGE_LO, "-4"),
    .address = 0x80652D00, .state = MSSB_GAME);

/* -4 here is the X change word, not the 0.0f: an original quirk, kept (see the doc). */
ASM(InputPrediction_BoxMoveY,
    BOX_MOVE_PREDICTION(IP_PREV_Y_CHANGE_LO, "-4"),
    .address = 0x80652D9C, .state = MSSB_GAME);
