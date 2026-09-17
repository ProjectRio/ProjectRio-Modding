/*###########################################################
# Default Competitive Rules
###########################################################*/
// Author: LittleCoaks
#include "Include/static/UnknownHomes_Static.h"

// r3 at the hook = the match settings block (0x803C5F04, no decomp symbol yet)
#define SETTING_INNINGS      0x3E
#define SETTING_MERCY        0x3F
#define SETTING_DROP_SPOT_P1 0x48
#define SETTING_DROP_SPOT_P2 0x4C

#define INNINGS_5 2
#define INNINGS_9 4

#define ROSTER_SLOTS_BOTH_TEAMS 18

CGECKO(DefaultCompetitiveRules, .address = 0x80049D08, .instruction = "lis r3, 0x803C",
       .notes = "Sets the competitive defaults on the match settings screen: mercy on;\n"
                "9 innings with drop spots on when no team has superstars,\n"
                "5 innings with drop spots off when superstars are used.");
void DefaultCompetitiveRules(void)
{
    READ_GAME_REG(u8*, settings, 3);

    const u8* starred = Static_Stats_Tables.charIsStarred;
    bool anyStars = false;
    for (int i = 0; i < ROSTER_SLOTS_BOTH_TEAMS; i++)
        if (starred[i] == 1)
            anyStars = true;

    settings[SETTING_INNINGS]      = anyStars ? INNINGS_5 : INNINGS_9;
    settings[SETTING_DROP_SPOT_P1] = !anyStars;
    settings[SETTING_DROP_SPOT_P2] = !anyStars;
    settings[SETTING_MERCY]        = 1;
}
