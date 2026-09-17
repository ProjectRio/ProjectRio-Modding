/*###########################################################
# Lock Thwomps Down
###########################################################*/
// Author: LittleCoaks
#include "Include/Rio/StadiumObjects.h"

#define THWOMP_DOWN_Y -5.0f

CGECKO(LockThwompsDown, .state = MSSB_GAME,
       .notes = "Locks the thwomps on Bowser's Castle in the down position.");
void LockThwompsDown(void)
{
    Thwomps_SetHeight(THWOMP_DOWN_Y);
}
