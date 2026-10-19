/*###########################################################
# All Teams Are 5-Star Teams
###########################################################*/
// Author: PeacockSlayer, LittleCoaks
#include "Include/static/UnknownHomes_Static.h"

#define TEAM_STARS 5

CGECKO(AllTeamsAre5StarTeams, .state = MSSB_GAME,
       .notes = "Whoever is on your team, it counts as a 5-star team.");
void AllTeamsAre5StarTeams(void)
{
    Static_Stats_Tables.startingChemStars[0] = TEAM_STARS;
    Static_Stats_Tables.startingChemStars[1] = TEAM_STARS;
}
