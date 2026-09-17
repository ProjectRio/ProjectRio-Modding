/*###########################################################
# Batter Lag Reduction & Positional Correction
###########################################################*/
// Author: LittleCoaks
// The prediction formula, the frame patches and every hook site: docs/match_codes.md
#include "Include/game/UnknownHomes_Game.h"

/* Rio-claimed words, see ClaimedFreeMemory.h. The ASM bodies reach them as
   lis 0x802E / ori, so the low halves are spelled out for them. */
#define PC_ZERO_F32       0x802EBF9C
#define PC_PREV_X_CHANGE  0x802EBFA0
#define PC_PREV_Y_CHANGE  0x802EBFA4
#define PC_PREV_X_POS     0x802EBFAC
#define PC_PREV_Y_POS     0x802EBFB0

#define CMPWI_R0_1 0x2C000001
#define CMPWI_R0_2 0x2C000002

/* game.rel bss byte the original code bumped alongside the swing frame; not in the decomp yet */
#define lbl_3_common_bss_32220_3 VAR_ADDRESS(u8, 0x80893303)

/*-----------------------------------------------------------
 Batter lag reduction
-----------------------------------------------------------*/

/* The original ini held its per-frame writes until the batter has a bat
   height, i.e. a match is really under way (see docs/match_codes.md). */
#define BatterIsPlaced() (*(u32*)&g_Batter.batPosition.y != 0)

/* Swing frame 1 can make contact, and the replay / camera "frame 1" tests move to frame 2. */
CGECKO(BatterLag_Patches, .state = MSSB_GAME,
       .notes = "For laggy displays or netplay: plays as if one\n"
                "frame of input delay were removed.\n"
                "Contact is judged as if the swing came a frame\n"
                "earlier, and the batter's box movement lands\n"
                "where a frame-earlier input would have put it.");
void BatterLag_Patches(void)
{
    if (!BatterIsPlaced())
        return;

    hittableFrameInd[0][1] = 1;
    PatchInstruction_Conditional(0x806A3688, CMPWI_R0_1, CMPWI_R0_2);   /* batterAnimations */
    PatchInstruction_Conditional(0x80654610, CMPWI_R0_1, CMPWI_R0_2);   /* assignFrameCountersToAPointer */
}

/* calculateIfHitBall entry: a swing on its first frame is judged as frame 2. */
CGECKO(BatterLag_ContactFrame, .address = 0x80651D48, .state = MSSB_GAME,
       .instruction = "lis r3, -0x7F77");
void BatterLag_ContactFrame(void)
{
    if (g_Batter.framesSinceStartOfSwing != 1)
        return;

    g_Batter.framesSinceStartOfSwing = 2;
    g_Batter.Stored_Frame_SwingContact_SinceMiss = 2;
    lbl_3_common_bss_32220_3 = 2;
}

/* ifSwing: frameSwung takes the pitch hangtime counter one frame early.
   ASM because the value has to come back in r0; r5 is reloaded right after. */
ASM(BatterLag_SwingFrame,
    "lha   0, 0x1B68(3)   \n"      /* the instruction we replace: g_Ball.pitchHangtimeCounter */
    "li    5, -1          \n"
    "cmpwi 0, 0xFFF       \n"
    "bgt   1f             \n"
    "add   0, 0, 5        \n"
    "1:                   \n"
    "nop                  \n",
    .address = 0x80652360, .state = MSSB_GAME);

/*-----------------------------------------------------------
 Positional correction (batterInBoxMovement, one hook per axis)
-----------------------------------------------------------*/

/* f0 = position, f2 = this frame's change; f0 must leave holding the result,
   so this stays ASM. r8, f12 and f13 are dead in this function at both sites.
   Base points at the axis' previous-change word: +0xC its previous position,
   ZERO_OFF the shared 0.0f. */
#define POSITIONAL_CORRECTION(BASE_LO, ZERO_OFF)                             \
    "lis   8, 0x802E              \n"                                        \
    "ori   8, 8, " BASE_LO "      \n"                                        \
    "lfs   12, 0xC(8)             \n"      /* previous position            */ \
    "fcmpu 0, 0, 12               \n"      /* moved by something else?     */ \
    "beq   1f                     \n"                                        \
    "lfs   12, " ZERO_OFF "(8)    \n"                                        \
    "stfs  12, 0(8)               \n"      /* then forget the last change  */ \
    "1:                           \n"                                        \
    "lfs   12, 0(8)               \n"      /* previous change              */ \
    "stfs  2, 0(8)                \n"      /* remember this one            */ \
    "lis   8, 0x4000              \n"                                        \
    "stw   8, -8(1)               \n"                                        \
    "lfs   13, -8(1)              \n"      /* 2.0f                         */ \
    "fmsub 2, 2, 13, 12           \n"      /* 2*change - previous change   */ \
    "fadds 0, 0, 2                \n"      /* the instruction we replace   */ \
    "lis   8, 0x802E              \n"                                        \
    "ori   8, 8, " BASE_LO "      \n"                                        \
    "stfs  0, 0xC(8)              \n"      /* new previous position        */

ASM(PositionalCorrection_X,
    POSITIONAL_CORRECTION("0xBFA0", "-4"),
    .address = 0x80652D00, .state = MSSB_GAME);

ASM(PositionalCorrection_Y,
    POSITIONAL_CORRECTION("0xBFA4", "-8"),
    .address = 0x80652D9C, .state = MSSB_GAME);
