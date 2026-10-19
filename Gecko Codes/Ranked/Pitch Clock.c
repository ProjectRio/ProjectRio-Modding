/*###########################################################
# Pitch Clock
###########################################################*/
// Author: LittleCoaks
#include "Include/game/UnknownHomes_Game.h"

#define NOP 0x60000000

// ASM: replaces the pitcher's A-button test, which reads and writes r0.
ASM(PitchClock,
    "lis    14, 0x8089       \n"
    "ori    14, 14, 0x0AE0   \n"   /* g_Pitcher.currentStateFrameCounter */
    "lhz    14, 0(14)        \n"
    "cmpwi  14, 600          \n"
    "bne    1f               \n"
    "li     0, 0x0100        \n"   /* clock expired: press A for the pitcher */
    "1:                      \n"
    "rlwinm. 0, 0, 0, 23, 23 \n",  /* the instruction we replace */
    .address = 0x806B4070, .state = MSSB_GAME,
    .notes = "Pitch clock: if the pitcher has not thrown within 10 seconds,\n"
             "the pitch is thrown automatically.");

// The pitcher state machine zeroes its frame counter on every state change; these
// three stores are the ones that would restart the clock without a pitch being thrown.
CGECKO(PitchClock_NoReset, .state = MSSB_GAME);
void PitchClock_NoReset(void)
{
    PatchInstruction_Conditional(0x806B4490, 0xB0030120, NOP);   /* sth r0, 0x120(r3)  */
    PatchInstruction_Conditional(0x806B42D0, 0xB1230120, NOP);   /* sth r9, 0x120(r3)  */
    PatchInstruction_Conditional(0x806B46B8, 0xB01E0120, NOP);   /* sth r0, 0x120(r30) */
}
