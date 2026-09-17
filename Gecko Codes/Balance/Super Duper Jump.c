/*###########################################################
# Super Duper Jump
###########################################################*/
// Author: PeacockSlayer, LittleCoaks
#include "Gecko Codes/Balance/SuperJump.h"

CGECKO(SuperDuperJump, .state = MSSB_GAME,
       .notes = "Characters with the Super Jump ability jump much higher.");
void SuperDuperJump(void)
{
    SetSuperJumpGravity(SUPER_DUPER_JUMP_GRAVITY);
}
