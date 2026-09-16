/*###########################################################
# Skip First Swing Frame
###########################################################*/
// Author: LittleCoaks
// How a swing counts, why a latch, and every hook site: docs/swing_frames.md
#include "Include/game/UnknownHomes_Game.h"
#include "Include/Rio/PatchTable.h"

#define SWING_LATCH       0x802EAF90    /* see ClaimedFreeMemory.h */
#define SWING_LATCH_HI     "0x802F"      /* lis / lbz -0x5070 reach it */
#define SWING_LATCH_LO     "-0x5070"
#define LATCH_REACTION_HELD 1            /* bit0: hit reaction already held  */
#define LATCH_ANIM_ARMED    2            /* bit1: swing animation already armed */

/* The counter increment. ASM because the result must come back in r0. Do NOT
   touch r3/r4 here: they hold half-loaded `lis` pairs. */
ASM(SkipFirstSwingFrame,
    "addi  0, 5, 1                    \n" /* the instruction we replace       */
    "cmpwi 5, 0                       \n"
    "bne   1f                         \n" /* not the first frame: leave it    */
    "addi  0, 5, 2                    \n" /* first frame: jump straight to 2  */
    "lis   11, " SWING_LATCH_HI      "\n"
    "li    12, 0                      \n"
    "stb   12, " SWING_LATCH_LO "(11) \n" /* new swing: re-arm both one-shots */
    "1:                               \n"
    "nop                              \n",
    .address = 0x80652374, .state = MSSB_GAME);

/* Hold the hit reaction for one frame so the swing animation can start. */
ASM(SkipFirstSwingFrame_LetSwingPlay,
    "cmplwi 0, 2                      \n" /* the instruction we replace       */
    "blt    1f                        \n" /* already the ordinary path        */
    "lis    11, 0x8089                \n"
    "lbz    12, 0x99D(11)             \n" /* g_Batter.swingInd                */
    "cmpwi  12, 0                     \n"
    "beq    2f                        \n"
    "lha    12, 0x976(11)             \n" /* g_Batter.framesSinceStartOfSwing */
    "cmpwi  12, 2                     \n"
    "bne    2f                        \n"
    "lis    11, " SWING_LATCH_HI     "\n"
    "lbz    12, " SWING_LATCH_LO "(11)\n"
    "andi.  12, 12, 1                 \n" /* held once already this swing?    */
    "bne    2f                        \n"
    "lbz    12, " SWING_LATCH_LO "(11)\n"
    "ori    12, 12, 1                 \n"
    "stb    12, " SWING_LATCH_LO "(11)\n"
    "li     12, 0                     \n" /* force "less than": hold the hit  */
    "cmpwi  12, 1                     \n" /* reaction for this one frame      */
    "b      1f                        \n"
    "2:                               \n"
    "li     12, 1                     \n" /* force "greater": react as the    */
    "cmpwi  12, 0                     \n" /* game intended                    */
    "1:                               \n"
    "nop                              \n",
    .address = 0x806A2D18, .state = MSSB_GAME);

/* Arm the swing animation once per swing off the latch; cr0 left as the
   original compare would leave it (equal = run the block). */
ASM(SkipFirstSwingFrame_ArmAnim,
    "lis   11, " SWING_LATCH_HI      "\n"
    "lbz   12, " SWING_LATCH_LO "(11) \n"
    "andi. 12, 12, 2                  \n" /* armed already this swing?        */
    "bne   1f                         \n" /* yes: leave cr0 not-equal, skip   */
    "lbz   12, " SWING_LATCH_LO "(11) \n"
    "ori   12, 12, 2                  \n"
    "stb   12, " SWING_LATCH_LO "(11) \n" /* (none of these touch cr0)        */
    "1:                               \n"
    "nop                              \n",
    .address = 0x806A3688, .state = MSSB_GAME);

/* The replay / camera `== 1` tests, moved to `== 2`. game.rel instructions,
   so they are re-patched per frame. */
#define CMPWI_R0_1 0x2C000001
#define CMPWI_R0_2 0x2C000002

static const RioPatch REPLAY_FRAME_ONE_TESTS[] = {
    { 0x80654504, CMPWI_R0_1, CMPWI_R0_2 },     /* fn_3_15458: camera / replay start frame      */
    { 0x80654610, CMPWI_R0_1, CMPWI_R0_2 },     /* assignFrameCountersToAPointer: replay stamp  */
    { 0x80654B70, CMPWI_R0_1, CMPWI_R0_2 },     /* fn_3_15A98: replay flag                      */
};

CGECKO(SkipFirstSwingFrame_Patches, .state = MSSB_GAME,
       .notes = "Every swing starts on frame 2 instead of\n"
                "frame 1, so swings are one frame faster.\n"
                "Frame 1 can never make contact anyway.");
void SkipFirstSwingFrame_Patches(void)
{
    RioPatch_Apply(REPLAY_FRAME_ONE_TESTS, RIO_PATCH_COUNT(REPLAY_FRAME_ONE_TESTS), CGECKO_ACTIVE);
}
