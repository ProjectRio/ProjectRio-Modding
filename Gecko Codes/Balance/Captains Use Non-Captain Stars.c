/*###########################################################
# Captains Use Non-Captain Stars
###########################################################*/
// Author: LittleCoaks
#include "Include/static/UnknownHomes_Static.h"

CGECKO(CaptainsUseNonCaptainStars,
       .notes = "Captains lose their captain star swings and pitches and use the regular ones instead.\n"
                "Side effect: the captain background during drafting is always Mario's.");
void CaptainsUseNonCaptainStars(void)
{
    for (int id = 0; id < NUM_CHOOSABLE_CHARACTERS; id++)
        Static_Stats_Tables.characterStats[id].stats.CaptainStarHitPitch = CAPTAIN_STAR_TYPE_NONE;
}
