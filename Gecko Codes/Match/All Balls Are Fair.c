/*###########################################################
# All Balls Are Fair
###########################################################*/
// Author: Nuche17
// Every fair/foul decision site and why each patch is enough: docs/match_codes.md
#include "Include/game/UnknownHomes_Game.h"
#include "Include/Rio/PatchTable.h"

#define LI_R3_0       0x38600000
#define LI_R29_0      0x3BA00000
#define CMPW_R3_R3    0x7C031800    /* cr0 = eq: "foul bit clear" */
#define CMPW_R20_R20  0x7C14A000
#define NOP           0x60000000

/* game.rel instructions, so they are re-patched per frame. */
static const RioPatch FAIR_BALL_PATCHES[] = {
    /* The geometry test: foul_checkIfFoul always returns FALSE. It lives in its
       own TU, so every caller reaches it through a bl and sees this. */
    { 0x806F6EB8, 0x38600001, LI_R3_0 },        /* foul_checkIfFoul: z < x - 0.15 arm       */
    { 0x806F6ED0, 0x54030FFE, LI_R3_0 },        /* foul_checkIfFoul: z < -x - 0.15 arm      */

    /* The collision mesh's foul-territory bit (BALL_COLLISION_TYPE_FOUL), tested
       inline rather than through foul_checkIfFoul. */
    { 0x80645758, 0x541D0630, LI_R29_0 },       /* checkForBallDead: isFoul = code & 0x80   */
    { 0x80648364, 0x54600631, CMPW_R3_R3 },     /* fairOrFoulBall: wall hit                 */
    { 0x80646C10, 0x56800631, CMPW_R20_R20 },   /* ballCollisionLogic: structure, in air    */
    { 0x8064780C, 0x56800631, CMPW_R20_R20 },   /* ballCollisionLogic: pit, no fielder yet  */

    /* The at-contact prediction. A predicted foul keeps the batter in the box,
       so skip the block that sets PREDICTED_HIT_OUTCOME_FOUL / _FOUL_BUNT. */
    { 0x8064B8C4, 0x41800014, NOP },            /* classifyHitTrajectory...: blt into block */
    { 0x8064B8D4, 0x408000E4, 0x480000E4 },     /* classifyHitTrajectory...: bge -> b past  */
};

CGECKO(AllBallsAreFair, .state = MSSB_GAME,
       .notes = "Every batted ball is fair. A ball that leaves\n"
                "the park in the air is a home run, even in\n"
                "foul territory.");
void AllBallsAreFair(void)
{
    RioPatch_Apply(FAIR_BALL_PATCHES, RIO_PATCH_COUNT(FAIR_BALL_PATCHES), CGECKO_ACTIVE);
}
