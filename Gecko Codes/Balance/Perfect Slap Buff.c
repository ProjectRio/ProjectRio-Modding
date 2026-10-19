/*###########################################################
# Perfect Slap Buff
###########################################################*/
// Author: MORI
#include "Include/mssbTypes.h"
#include "Include/Symbols/game.h"

typedef struct { s16 powerLower; s16 powerUpper; s16 addedGravity; } hitball_power_range;
#define BallHitArray ARRAY_2D_ADDRESS(hitball_power_range, 2, 5, BallHitArray_ADDR)

#define PERFECT_SLAP_POWER_LOWER   165   /* stock 145 */
#define PERFECT_SLAP_POWER_UPPER   180   /* stock 150 */
#define PERFECT_SLAP_ADDED_GRAVITY (-75) /* stock -110 */

CGECKO(PerfectSlapBuff, .state = MSSB_GAME,
       .notes = "Perfect-contact slap hits are hit harder.");
void PerfectSlapBuff(void)
{
    hitball_power_range* perfectSlap = &BallHitArray[BAT_CONTACT_TYPE_SLAP][HIT_CONTACT_TYPE_PERFECT];
    perfectSlap->powerLower   = PERFECT_SLAP_POWER_LOWER;
    perfectSlap->powerUpper   = PERFECT_SLAP_POWER_UPPER;
    perfectSlap->addedGravity = PERFECT_SLAP_ADDED_GRAVITY;
}
