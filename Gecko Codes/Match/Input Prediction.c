/*###########################################################
# Input Prediction
###########################################################*/
// Author: LittleCoaks
// Pitching half: docs/import_match_rules.md. Batting half: BattingPrediction.h

#define BATTING_PREDICTION_NOTES                           \
    "Predicts your inputs while pitching, batting and\n"   \
    "moving in the batter's box, to hide input delay.\n"   \
    "For laggy netplay or a slow TV.\n"                    \
    "Not for use with Auto Golf Mode."

#include "Gecko Codes/Match/BattingPrediction.h"
#include "Include/game/pitching/pitcher.h"

/* Rio-claimed words, see ClaimedFreeMemory.h */
#define IP_PREV_CURVE_INPUT  VAR_ADDRESS(u8,  0x802EBF98)
#define IP_CURVE_VELOCITY    VAR_ADDRESS(f32, 0x802EBFB4)

#define PORT_COUNT 4

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
       .instruction = "lwz r0, 0x14(r1)");
void InputPrediction_StoreCurveInput(void)
{
    IP_PREV_CURVE_INPUT = CurveInput(g_FieldingLogic.fielderInputs);
}

/* Right after the pitch update's pitchCurve() call. */
CGECKO(InputPrediction_PitchCurve, .address = 0x806B05C4, .state = MSSB_GAME,
       .instruction = "lis r3, -0x7F77");
void InputPrediction_PitchCurve(void)
{
    u32 port = g_GameLogic.teams[g_GameLogic.teamFielding];

    if (port >= PORT_COUNT)
        return;

    IP_CURVE_VELOCITY = g_Pitcher.pitchCurveVeloV1;
    if (CurveInput(g_ControlsByPort[port].buttonInput) != IP_PREV_CURVE_INPUT)
    {
        pitchCurve();
        IP_CURVE_VELOCITY += g_Pitcher.pitchCurveVeloV1;
    }
    g_Pitcher.pitchCurveVeloV1 = IP_CURVE_VELOCITY;
}
