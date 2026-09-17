/*###########################################################
# Hold Up for Eephus
###########################################################*/
// Author: LittleCoaks
// The two pitch-speed reads and the hook sites: docs/import_match_gameplay.md
#include "Include/game/UnknownHomes_Game.h"

#define EEPHUS_SPEED 0x41

static inline BOOL PitcherHoldsUp(void)
{
    return (g_FieldingLogic.fielderInputs & INPUT_BUTTON_UP) != 0;
}

CGECKO(HoldUpForEephus_Curve, .address = 0x806B1F34, .state = MSSB_GAME,
       .notes = "Hold up on the control stick as the pitcher to lob the ball high in the air.");
void HoldUpForEephus_Curve(void)
{
    READ_GAME_REG(InMemPitcherType*, pitcher, 31);
    pitcher->pitchSpeed = PitcherHoldsUp() ? EEPHUS_SPEED : pitcher->curveBallSpeed;
}

CGECKO(HoldUpForEephus_Fast, .address = 0x806B1E58, .state = MSSB_GAME,
       .instruction = "stw r5, 0x1C(r1)");
void HoldUpForEephus_Fast(void)
{
    if (PitcherHoldsUp())
        WRITE_GAME_REG(5, EEPHUS_SPEED);
}
