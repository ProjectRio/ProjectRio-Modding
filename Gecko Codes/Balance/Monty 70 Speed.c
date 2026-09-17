/*###########################################################
# Monty 70 Speed
###########################################################*/
// Author: MattGree
#include "Include/static/UnknownHomes_Static.h"

#define MONTY_SPEED 70

CGECKO(Monty70Speed,
       .notes = "Sets Monty Mole's speed to 70.");
void Monty70Speed(void)
{
    Static_Stats_Tables.characterStats[CHAR_ID_MONTY].stats.Speed = MONTY_SPEED;
}
