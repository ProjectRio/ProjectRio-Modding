/*###########################################################
# Skip First Swing Frame
###########################################################*/
// Author: LittleCoaks
// *Every swing starts on frame 2 instead of frame 1, so it is one frame faster.
// *Frame 1 of a swing can never make contact (slap swings are hittable on
// *frames 2-10, charge swings on 3-9), so nothing is lost but the wait.
#include "Include/game/UnknownHomes_Game.h"

/* HOW A SWING COUNTS. ifSwing (game.rel 0x806522F0) runs every frame while
   g_Batter.swingInd is set and does `framesSinceStartOfSwing++` before any of
   the contact / sound / animation logic looks at the counter. It is 0 on the
   frame the swing starts, so the first pass through makes it 1, the next 2,
   and so on. The hittable window (hittableFrameInd) opens at 2.

   THE MOD. On that first pass, add 2 instead of 1: the counter goes 0 -> 2 and
   the swing lands straight in the hittable window. Every later pass is the
   plain +1, so the rest of the swing (sound on frame 5/6, animation end at
   frameDelay) is unchanged and merely arrives a frame sooner.

   Injected on the `addi r0, r5, 1` itself (0x80652374; r5 = the old counter,
   r0 = the new one, stored two instructions later). ASM rather than C because
   the result has to come back in r0, which the C2-in-C wrapper cannot deliver
   (it clobbers r0 and reloads it with the saved LR on the way out). cr0 is free
   here: nothing between this and the next compare reads it, and the call two
   instructions down would trash it anyway. r11/r12 are the scratch. Do NOT
   reach for r3/r4 despite the `addi r4, r4, 0x910` / `addi r3, r3, 0x5D38`
   right below -- those only complete `lis` halves loaded two instructions ABOVE
   the hook, so writing r3/r4 here turns g_Batter into 0x910 and the
   `stb r0, 0x9B(r4)` below into a wild store into the OS area.

   The hook also clears SWING_LATCH, because this is the one place that knows a
   NEW swing has begun. See below for what that latch is for. */
#define SWING_LATCH        0x802EAF90    /* see ClaimedFreeMemory.h */
#define SWING_LATCH_HI     "0x802F"      /* lis / lbz -0x5070 reach it */
#define SWING_LATCH_LO     "-0x5070"
#define LATCH_REACTION_HELD 1            /* bit0: hit reaction already held  */
#define LATCH_ANIM_ARMED    2            /* bit1: swing animation already armed */

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

/* WHY A LATCH AND NOT A COUNTER VALUE.

   Everything below wants to happen exactly once per swing, and the obvious
   trigger -- `framesSinceStartOfSwing == 2`, the mod's replacement for the
   game's own `== 1` -- is not actually once. When the ball is hit on the
   swing's first frame the play moves on and atBat_batter stops being called, so
   ifSwing never runs again and the counter sits at 2 with swingInd still set
   for the rest of the play. Every `== 2` test then fires on every frame:
   measured, the swing animation re-armed and restarted continuously, the actor
   flickering between 0x4D and its follow-through 0x50. Vanilla never has this
   problem because the value it freezes at is never 1.

   So the two animation edges below are driven off a one-shot latch instead,
   cleared by the counter hook above on the frame a swing starts. */

/* LETTING THE SWING ANIMATION HAPPEN AT ALL.

   simulate1FrameOfTheGame (0x80699D34) runs baseballMatchSimulation -- which
   contains ifSwing -- BEFORE matchAnimations -> batterAnimations. So on the
   swing's first frame the counter already reads 2 when the animation code looks
   at it, and that is now also the first frame contact can happen. And on the
   frame the ball is hit, batterAnimations never gets as far as the swing: at
   0x806A2D18 it tests the play state, takes the hit-reaction branch (a switch
   on g_Batter.hitTrajectory that plays 0x63 / 0x64 / 0x51 ...) and RETURNS.
   Vanilla survives that because its swing animation was set a frame earlier;
   with the mod the two land on the same frame and the swing loses. Measured on
   a real frame-2 hit: the batter goes straight from its pre-pitch animation
   0x56 to the hit reaction 0x63, with no swing in between.

   So for one frame per swing the hit reaction is held back and the ordinary
   animation path runs instead -- which reaches the swing block, arms, and
   starts the swing exactly as on any other swing. The next frame the latch is
   set and the reaction goes ahead as the game intended.

   0x806A2D18 is `cmplwi r0, 2`, feeding the `blt` to the ordinary path on the
   next instruction, so forcing that compare is all it takes. r0 is untouched,
   and r11/r12 do not appear anywhere in batterAnimations. */
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

/* ARMING THE SWING ANIMATION. batterAnimations+0xAEC (0x806A3688) is the
   `cmpwi r0, 1` that decides whether to work out the animation's start delay
   and clear its "already playing" flag -- the block just below it, which the
   code after that turns into the actual AnimateCharacter call. The whole thing
   already sits inside `if (g_Batter.swingInd)`, so the counter value was only
   ever the game's way of saying "first frame of this swing". The latch says
   that properly, and unlike the counter it cannot get stuck. cr0 is left
   exactly as the original compare would have left it: equal = run the block. */
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

/* THE REPLAY / CAMERA EDGES. Three more `framesSinceStartOfSwing == 1` tests
   exist in game.rel -- these and the animation one above are all of them, found
   by walking every halfword load at g_Batter+0x66 and keeping the ones whose
   base register resolves to g_Batter:

   0x80654504  fn_3_15458+0x18          camera: stamps the swing's start frame
       for the replay and raises the camera's swing flag.
   0x80654610  assignFrameCountersToAPointer+0x58 and
   0x80654B70  fn_3_15A98+0x44          the other two replay-recorder stamps
       off the same edge (halfword at +0xA76, flag at +0xAB5).

   These are pure bookkeeping, so they just move to 2 rather than getting their
   own latch bits. The frozen-counter case above applies to them too: on a swing
   that connects on the very first hittable frame they re-stamp every frame for
   the rest of the play. That is invisible in play and only touches replay
   bookkeeping, but it is the known rough edge here.

   Patched per frame, because these are game.rel instructions and have to be
   re-applied every load. PatchInstruction_Conditional only writes when the
   expected word is there, so ON and OFF are the same call with the last two
   arguments swapped, and both are idempotent. */
#define CMPWI_R0_1 0x2C000001
#define CMPWI_R0_2 0x2C000002

static const u32 REPLAY_FRAME_ONE_TESTS[] = {
    0x80654504,     /* fn_3_15458: camera / replay start frame      */
    0x80654610,     /* assignFrameCountersToAPointer: replay stamp  */
    0x80654B70,     /* fn_3_15A98: replay flag                      */
};
#define N_TESTS ((int)(sizeof(REPLAY_FRAME_ONE_TESTS) / sizeof(REPLAY_FRAME_ONE_TESTS[0])))

CGECKO(SkipFirstSwingFrame_Patches, .state = MSSB_GAME,
       .notes = "Every swing starts on frame 2 instead of\n"
                "frame 1, so swings are one frame faster.\n"
                "Frame 1 can never make contact anyway.");
void SkipFirstSwingFrame_Patches(void)
{
    bool on = CGECKO_ACTIVE;
    int i;

    for (i = 0; i < N_TESTS; i++)
    {
        if (on)
            PatchInstruction_Conditional(REPLAY_FRAME_ONE_TESTS[i], CMPWI_R0_1, CMPWI_R0_2);
        else
            PatchInstruction_Conditional(REPLAY_FRAME_ONE_TESTS[i], CMPWI_R0_2, CMPWI_R0_1);
    }
}
