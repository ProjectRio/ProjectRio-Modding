/*###########################################################
# Hazardless w Star Pads on Bowser's Castle
###########################################################*/
// Author: LittleCoaks
// Same hook and patches as Hazardless Stadiums (docs/match_codes.md); what differs: docs/import_match_rules.md
#include "Include/static/UnknownHomes_Static.h"
#include "Include/Symbols/game.h"
#include "Include/Rio/StadiumObjects.h"

#define NOP      0x60000000
#define LI_R0_0  0x38000000
#define LI_R0_7  0x38000007
#define LI_R3_0  0x38600000

#define OFF_FIELD_F32     0x42C80000   /* 100.0f */
#define THWOMP_BURIED_Y   50.0f

CGECKO(HazardlessStarPads, .address = 0x80699508, .state = MSSB_GAME,
       .instruction = "lbz r5, 9(r4)",
       .notes = "Removes the hazards from every stadium, but keeps\n"
                "the star pads on Bowser's Castle.");
void HazardlessStarPads(void)
{
    switch (g_d_GameSettings.StadiumID)
    {
    case STADIUM_ID_WARIO_PALACE:
        PatchInstruction(0x8070FC30, NOP);          /* palaceNadoLogic */
        PatchInstruction(0x8071339C, LI_R0_0);      /* palaceChainChompControl */
        VAR_ADDRESS(u32, TornadoPlacementConfig_ADDR + 0x00) = OFF_FIELD_F32;
        VAR_ADDRESS(u32, TornadoPlacementConfig_ADDR + 0x34) = OFF_FIELD_F32;
        break;

    case STADIUM_ID_BOWSERS_CASTLE:
        PatchInstruction(0x807056C8, LI_R3_0);      /* flameControl */
        PatchInstruction(0x80706D00, LI_R0_0);      /* thwomp_slamControl */
        break;

    case STADIUM_ID_YOHSI_PARK:
        PatchInstruction(0x80724428, LI_R0_7);      /* loadYoshiPark */
        break;

    case STADIUM_ID_PEACH_GARDEN:
        PatchInstruction(0x80739A28, LI_R0_7);      /* loadPeachGarden */
        break;

    case STADIUM_ID_DK_JUNGLE:
        PatchInstruction(0x80736D08, LI_R0_7);      /* loadDKJungle */
        PatchInstruction(0x807343A4, NOP);          /* handleBarrelFiring */
        PatchInstruction(0x807343B0, NOP);
        break;
    }
}

CGECKO(HazardlessStarPads_BuryThwomps, .state = MSSB_GAME);
void HazardlessStarPads_BuryThwomps(void)
{
    if (g_d_GameSettings.StadiumID == STADIUM_ID_BOWSERS_CASTLE)
        Thwomps_SetHeight(THWOMP_BURIED_Y);
}
