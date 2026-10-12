/*###########################################################
# Teams Exhibition
###########################################################*/
// Author: LittleCoaks
// Rotation rules and the HUD icon rewrite: docs/teams_exhibition.md

#include "Include/game/UnknownHomes_Game.h"

#include "Include/static/UnknownHomes_Static.h"
#include "Include/Unknown/File_0x800b0a14.h"
/* The synced header types g_Controls as a pointer; the game keeps four InputStructs here. */
#define g_ControlsByPort ARRAY_1D_ADDRESS(InputStruct, 4, 0x80893928)
/* Set from contact until the play ends; the Rio client's autogolf switches to the fielder on it. */
#define ball_in_play VAR_ADDRESS(u8, 0x8089389B)

// replay_atBat counts as at-bat: the HUD icon hooks can run during replays
static inline __attribute__((always_inline))
bool IsAtBatScene(void)
{
    int scene = g_GameLogic.sceneID;
    return (scene == SCENE_ID_AT_BAT) || (scene == SCENE_ID_REPLAY_AT_BAT);
}

// Which physical ports (0-3) should have control; false when this isn't a
// supported teams game. is_at_bat picks the pitcher or the live-ball fielder.
static inline __attribute__((always_inline))
bool GetActivePorts(int* fielder_port_out, int* batter_port_out, bool is_at_bat)
{
    int second_drafter = g_d_GameSettings.PlayerPorts[1];

    // 1v2 (1), 2v2 (2), and 1v3 (3) are supported; vs-CPU / anything else -> inactive
    if (second_drafter < 1 || second_drafter > 3)
        return false;

    // team 1's base is team 0's size, NOT PlayerPorts[1]
    int team_base[2];
    int team_size[2];
    team_base[0] = 0;
    team_size[0] = (second_drafter == 2) ? 2 : 1;   // 2v2 -> team 0 has two; else solo
    team_base[1] = team_size[0];
    team_size[1] = (second_drafter == 3) ? 3 : 2;   // 1v3 -> team 1 has three; else two

    int field_team = g_GameLogic.teamFielding;
    int bat_team   = g_GameLogic.teamBatting;

    // plateAppearances (unlike AtBats) counts walks
    int number_PAs = 0;
    for (int i = 0; i <= 8; i++)
    {
        number_PAs += BatterStats_P1_P2[bat_team][i].plateAppearances;
    }

    // fielding
    int field_base = team_base[field_team];
    int field_size = team_size[field_team];
    int fielder_port;
    if (field_size == 1)
    {
        // solo defender pitches AND fields
        fielder_port = field_base;
    }
    else if (field_size == 2)
    {
        // two defenders split pitcher/fielder and trade roles each inning
        bool is_odd_inning = g_Scores.Inning % 2 == 1;
        bool use_fielder_a = (is_odd_inning == is_at_bat);
        fielder_port = use_fielder_a ? field_base : field_base + 1;
    }
    else
    {
        // three defenders: rotate the pitcher/fielder pair each batter faced; the third rests
        int r = number_PAs % 3;
        int pitcher_port = field_base + r;
        int fielder2_port = field_base + (r + 1) % 3;
        fielder_port = is_at_bat ? pitcher_port : fielder2_port;
    }
    *fielder_port_out = fielder_port;

    // batting -- round-robin through the batting team's players each plate appearance
    *batter_port_out = team_base[bat_team] + (number_PAs % team_size[bat_team]);

    return true;
}

/*-----------------------------------------------------------
 Section 1: hand over control (end of UpdateControllerInputs)
-----------------------------------------------------------*/

CGECKO(TeamsExhibition, .address = 0x806AC530, .state = MSSB_GAME,
                        .instruction = "blr",
                        .notes = "Allows 1v2, 2v2, and 1v3 team matches in exhibition mode\n"
                                 "Batting:  swaps every plate appearance.\n"
                                 "Fielding: swaps every inning (2 players) or every batter (3 players)\n\n"
                                 "Game type is based on who drafts:\n"
                                 "  Port 2 -> 1v2 (P1 vs P2/P3)\n"
                                 "  Port 3 -> 2v2 (P1/P2 vs P3/P4)\n"
                                 "  Port 4 -> 1v3 (P1 vs P2/P3/P4)\n"
                                );
void TeamsExhibition()
{
    int fielder_port, batter_port;
    if (!GetActivePorts(&fielder_port, &batter_port, ball_in_play != 1))
        return;

    int read_port[2];
    read_port[0] = 0;
    read_port[1] = g_d_GameSettings.PlayerPorts[1];

    // replays play recorded inputs back through the drafting ports: hand those back, copy nothing
    int scene = g_GameLogic.sceneID;
    if (scene == SCENE_ID_REPLAY_AT_BAT || scene == SCENE_ID_REPLAY_LIVE_BALL)
    {
        g_GameLogic.teams[0] = read_port[0];
        g_GameLogic.teams[1] = read_port[1];
        return;
    }

    int field_team = g_GameLogic.teamFielding;
    int bat_team   = g_GameLogic.teamBatting;
    int active_port[2];
    active_port[field_team] = fielder_port;
    active_port[bat_team]   = batter_port;

    // the game and the Rio client both take each team's port from here
    g_GameLogic.teams[0] = active_port[0];
    g_GameLogic.teams[1] = active_port[1];

    // anything still reading the drafting ports gets the active teammate's inputs too
    for (int t = 0; t < 2; t++)
    {
        int src = active_port[t];
        int dst = read_port[t];
        if (src == dst)
            continue;
        // InputStruct is 0x10 bytes; copy as 4 words to avoid a memcpy call
        u32* s = (u32*)&g_ControlsByPort[src];
        u32* d = (u32*)&g_ControlsByPort[dst];
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
    }
}

/*-----------------------------------------------------------
 Section 2: HUD port icons (draw_ongoingStarGuageHud)
-----------------------------------------------------------*/

#define hud_slot_table 0x80371C30

CGECKO(TeamsPortIcons_Ongoing, .address = 0x806D8EC0, .state = MSSB_GAME,
                               .instruction = "li r5, -1");
void TeamsPortIcons_Ongoing()
{
    int fielder_port, batter_port;
    if (!GetActivePorts(&fielder_port, &batter_port, IsAtBatScene()))
        return;

    u8* mgr = (u8*)currentDrawingItem;
    if (!mgr)
        return;
    int idx = *(u16*)(mgr + 0x14);
    u8* entry = (u8*)(hud_slot_table + (idx << 3));

    u8* field_obj = *(u8**)(entry + 0x78);
    u8* bat_obj   = *(u8**)(entry + 0x80);
    if (field_obj)
        *(u32*)(field_obj + 0x5C) = (u32)fielder_port << 16;
    if (bat_obj)
        *(u32*)(bat_obj + 0x5C)   = (u32)batter_port << 16;
}
