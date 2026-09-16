/*###########################################################
# Test New Boot Match
###########################################################*/
// Author: LittleCoaks
// Payload layout, field semantics and the boot state machine: docs/boot_to_match.md

#include "Include/game/UnknownHomes_Game.h"

#include "Include/static/UnknownHomes_Static.h"
#include "Include/Unknown/File_0x80065dec.h"
#include "Include/Unknown/File_0x800671fc.h"
#include "Include/Unknown/File_0x80069854.h"
#include "Include/Unknown/File_0x80064754.h"
#include "Include/Unknown/File_0x800678cc.h"
#include "Include/Unknown/File_0x80064a04.h"
#include "Include/menus/yd_step.h"
#include "Include/menus/text_0323C.h"
#include "Include/musyx/musyx.h"

// game-REL object bound by address (Include/Symbols won't bind both RELs); only valid once game.rel is resident
#define g_Scores VAR_ADDRESS(GameScoresControlsStruct, 0x808928A0)

// Rosters and per-player arrays are in draft-slot order.
typedef struct BootMatchSpec
{
    u8 captain[2];            // charID of each team's captain
    u8 roster[2][9];          // charIDs
    u8 battingHand[2][9];     // 0 = right, 1 = left
    u8 fieldingHand[2][9];    // 0 = right, 1 = left
    u8 superstar[2][9];       // 0 = off, 1 = on
    u8 captainOrderLoc[2];    // captain's slot in the batting order, 0-8
    u8 logo[2];               // team logo id, 0-0x2F

    u8 stadiumCursor;         // stadium-select CURSOR index (0=Mario..5=DK), not the stadium id; no Toy Field
    u8 firstBatter;           // 0 = P1 bats first, 1 = P2 (becomes the in-game away side)
    u8 starSkills;            // 0 = off, 1 = on
    u8 innings;               // actual inning count
    u8 mercy;                 // 0 = off, 1 = on

    u8 isCpuMatch;            // 1 = P1 vs CPU (UNVERIFIED; the tested path is two humans)
    u8 p2Port;                // physical controller port for the second human, 2-4 (1-based)
} BootMatchSpec;

// Where in a game to land. Applied every pre-pitch frame because game init
// overwrites these values partway through the load.
typedef struct BootStateConfig
{
    u8 apply_state;        // 0 = fresh match; 1 = jump to the state below
    u8 inning;             // current inning, 1-based; keep <= the spec's `innings`
    u8 bottom_of_inning;   // 0 = top, 1 = bottom
    u16  score_away;         // total runs (also written to inning 1's box)
    u16  score_home;
    u8 balls;              // 0-3
    u8 strikes;            // 0-2
    u8 outs;               // 0-2
    u8 stars_p1;           // 0-5
    u8 stars_p2;
    u8 star_chance;        // 0 = off, 1 = star chance active

    // Batting order + fielding positions, AWAY/HOME order, current-batter-first.
    u8 apply_order;           // 0 = leave the game's default order alone
    u8 orderChar_A_H[2][9];   // charID batting in each slot
    u8 orderPos_A_H[2][9];    // fielding position in each slot

    u8 currentBatter_A_H[2];  // current batter's slot, 1-indexed (away, home)

    // Per natural batting slot, P1/P2-indexed.
    u8 orderFieldHand_P1P2[2][9]; // 0 = right, 1 = left
    u8 orderBatHand_P1P2[2][9];   // 0 = right, 1 = left
    u8 orderSuperstar_P1P2[2][9]; // 0 = off, 1 = on

    // Runners on base: index 0 = 1B, 1 = 2B, 2 = 3B.
    u8 runner_present[3];
    u8 runner_rosterLoc[3];
    u8 runner_charID[3];

    u8 positionSwap[2][9];    // positionSwapMapping, draft-slot order (identity does NOT work)
} BootStateConfig;

// The Rio client patches this blob in place by scanning for `magic`; any layout
// change here must be mirrored in the client. See docs/boot_to_match.md.
typedef struct BootMatchPayload
{
    u32            magic;      // BOOT_PAYLOAD_MAGIC ('RIOB')
    BootMatchSpec   spec;       // payload offset 4
    BootStateConfig config;     // payload offset 90 (spec is 85 B + 1 pad for u16 align)
} BootMatchPayload;

#define BOOT_PAYLOAD_MAGIC 0x52494F42  /* 'RIOB' */

static const BootMatchPayload boot_payload =
{
    .magic = BOOT_PAYLOAD_MAGIC,
    .spec =
    {
        .captain = { CHAR_ID_BOWSER, CHAR_ID_PEACH },
        .roster = {
            { CHAR_ID_BOWSER, CHAR_ID_PEACH, CHAR_ID_DRYBONES_RED, CHAR_ID_BIRDO, CHAR_ID_TOAD_BLUE, CHAR_ID_LUIGI, CHAR_ID_KOOPA_RED, CHAR_ID_PARATROOPA_RED, CHAR_ID_MAGIKOOPA_BLUE },
            { CHAR_ID_PEACH, CHAR_ID_PEACH, CHAR_ID_TOADETTE, CHAR_ID_NOKI_BLUE, CHAR_ID_PIANTA_RED, CHAR_ID_YOSHI, CHAR_ID_MONTY, CHAR_ID_SHYGUY_RED, CHAR_ID_BOO },
        },
        .battingHand      = { { 0,1,0,1,0,1,0,0,0 }, { 1,0,1,0,0,0,0,0,0 } },
        .fieldingHand     = { { 0,1,0,1,0,1,0,0,0 }, { 1,0,1,0,0,0,0,0,0 } },
        .superstar        = { { 1,0,0,0,1,0,0,0,0 }, { 0,1,0,0,0,0,0,0,1 } },
        .captainOrderLoc  = { 3, 6 },   // captains bat 4th (P1) and 7th (P2)
        .logo             = { 5, 12 },
        .stadiumCursor    = 3,      // 0=Mario 1=Bowser 2=Wario 3=Yoshi 4=Peach 5=DK
        .firstBatter      = 1,      // P2 bats first
        .starSkills       = 1,      // ON
        .innings          = 5,
        .mercy            = 1,      // ON

        .isCpuMatch       = 0,      // two humans (the tested force-swap path)
        .p2Port           = 2,      // second human on controller port 2
    },

    .config =
    {
        .apply_state      = 1,
        // MUST be <= the spec's `innings`.
        .inning           = 3,
        .bottom_of_inning = 0,      // TOP of the inning
        .score_away       = 8,
        .score_home       = 3,
        .balls            = 3,      // full count
        .strikes          = 2,
        .outs             = 2,
        .stars_p1         = 5,
        .stars_p2         = 2,
        .star_chance      = 1,

        // Placeholder order chosen so each captain lands at captainOrderLoc {3,6}
        // above, keeping the placeholder self-consistent for the Client's self-test.
        .apply_order      = 1,
        .orderPos_A_H  = { { 0,2,3,4,5,6,1,7,8 }, { 1,2,3,0,4,5,6,7,8 } },
        .orderChar_A_H = { { 0x04,0x0F,0x18,0x16,0x06,0x12,0x04,0x10,0x0E },
                           { 0x04,0x32,0x11,0x09,0x1D,0x01,0x2A,0x14,0x21 } },
        .currentBatter_A_H = { 1, 1 },
        .orderFieldHand_P1P2 = { { 1,0,1,0,0,1,0,0,0 }, { 1,1,0,0,0,0,0,0,0 } },
        .orderBatHand_P1P2   = { { 1,0,1,0,0,1,0,0,0 }, { 1,1,0,0,0,0,0,0,0 } },
        .orderSuperstar_P1P2 = { { 0,0,0,1,1,0,0,0,0 }, { 0,0,0,0,0,0,1,0,1 } },
        .runner_present   = { 0, 0, 0 },   // no runners in the placeholder
        .runner_rosterLoc = { 0, 0, 0 },
        .runner_charID    = { 0, 0, 0 },
        // placeholder roster is in position order, so positionSwap is identity
        .positionSwap = { { 0,1,2,3,4,5,6,7,8 }, { 0,1,2,3,4,5,6,7,8 } },
    },
};

// Launder the payload pointer so -O1 cannot constant-fold its fields into
// immediates, which would make the Client's byte patching a no-op.
#define OPAQUE_PTR(p) __asm__("" : "+r"(p))

// menu->game rel swap request (loader node 0x80111300 + 0x10)
#define trigger_rel_change VAR_ADDRESS(short, 0x80111310)
// claimed free memory: post-start outs-burst frame counter (shared with Boot Directly To Game; never run together)
#define tbm_outsCounter VAR_ADDRESS(u8, 0x802EC01A)

// Runner-on-base staging (1B/2B/3B); the clear instruction is nop'd during the load and restored after.
#define tbm_runnerRosterLoc(i)  VAR_ADDRESS(u16,  0x8088F04C + (i) * 0x154)
#define tbm_runnerCharID(i)     VAR_ADDRESS(u16,  0x8088F04E + (i) * 0x154)
#define tbm_runnerClearInstr(i) VAR_ADDRESS(u32, 0x806C9420 + (i) * 0x30)
static inline u32 tbm_runnerRestoreInstr(int i) { return (i == 0) ? 0xB0650234 : 0xB06500E0; }

// Outs are applied a few seconds after the match starts: booting with outs on the board silences the first at-bat's BGM.
#define TBM_OUTS_DELAY_FRAMES 180
#define TBM_OUTS_BURST_FRAMES 10


// Stage the predetermined match and request the swap. Runs once: it flips
// `rel` to 5 up front, so the per-frame guard keeps it from re-entering.
static inline void TestBoot_StageMatch(const BootMatchSpec* s, const BootStateConfig* c)
{
    inningSetting.rel = 5;
    trigger_rel_change = 1;
    sndFXStartEx(0x1bb, 0x40, 0x3f, 0x0); // rio bat SFX -- signals the boot

    if (s->isCpuMatch)
    {
        // UNVERIFIED path. One human on port 1, CPU opponent.
        g_d_GameSettings.p2_CPU_match_code = P2_CPU_CODE_1_PLAYER_GAME;
        Static_Stats_Tables.playerNumberByPort[0] = 0;
        Static_Stats_Tables.portsActiveInMatch[0] = 0;      // 0 = active
        Static_Stats_Tables.portsActiveInMatch[1] = 0xFF;
        Static_Stats_Tables.portsActiveInMatch[2] = 0xFF;
        Static_Stats_Tables.portsActiveInMatch[3] = 0xFF;
        Static_Stats_Tables.player2Ind = 0;
        g_MatchInfo.player2Ind2                    = 0;
    }
    else
    {
        g_d_GameSettings.p2_CPU_match_code = P2_CPU_CODE_2_PLAYER_GAME;
        Static_Stats_Tables.playerNumberByPort[0] = 0;      // P1 = port 1
        Static_Stats_Tables.playerNumberByPort[1] = (u8)(s->p2Port - 1);
        Static_Stats_Tables.portsActiveInMatch[0] = 0;
        Static_Stats_Tables.portsActiveInMatch[1] = 0;
        Static_Stats_Tables.portsActiveInMatch[2] = 0xFF;
        Static_Stats_Tables.portsActiveInMatch[3] = 0xFF;
        Static_Stats_Tables.player2Ind = 1;
        g_MatchInfo.player2Ind2                    = 1;
    }

    Static_Stats_Tables.captainSelectedID[0] = s->captain[0];
    Static_Stats_Tables.captainSelectedID[1] = s->captain[1];

    for (int i = 0; i < 54; i++)
        Static_Stats_Tables.charOnCharacterGridSelected[i] = 0;
    for (int team = 0; team < 2; team++)
        for (int slot = 0; slot < 9; slot++)
            Static_Stats_Tables.charOnCharacterGridSelected[s->roster[team][slot]] = 1;

    // positionSwapMapping must be set before the chain (characterSelectScreen reads it)
    for (int team = 0; team < 2; team++)
    {
        for (int slot = 0; slot < 9; slot++)
        {
            cursorPositions.roster.rosterCharID[team][slot]        = s->roster[team][slot];
            cursorPositions.roster.positionSwapMapping[team][slot] = c->positionSwap[team][slot];
            cursorPositions.roster.rosterSpotFilledInd[team][slot] = 1;
        }
    }

    // conversion chain, once, in loadDemoMatch's order
    copyInfoToInMemRoster();
    teamLogoDetermination(0);
    teamLogoDetermination(1);
    unsure_FillRosterPositions(0);
    unsure_FillRosterPositions(1);
    characterSelectScreen(0);
    characterSelectScreen(1);
    setCaptainLocInRoster();

    g_d_GameSettings.StadiumID = (u8)cursorToStadIDMapping[s->stadiumCursor];

    g_d_GameSettings.home_AwaySetting            = s->firstBatter;   // 0 = P1 away/home
    inningSetting.inningCount               = s->innings;
    inningSetting.starSkillsSetting  = s->starSkills;
    inningSetting.runsNeededForMercy          = s->mercy ? 10 : 0; // MSSB mercy = 10 runs; 0/off UNVERIFIED

    setPortOfEachPlayer();

    // handedness / superstar are applied per batting slot in TestBoot_ApplyGameState, after the reorder
    Static_Stats_Tables.capLocationInOrder[0] = s->captainOrderLoc[0];
    Static_Stats_Tables.capLocationInOrder[1] = s->captainOrderLoc[1];
    Static_Stats_Tables.teamName[0]           = s->logo[0];
    Static_Stats_Tables.teamName[1]           = s->logo[1];

    tbm_outsCounter = 0;
}

// Land the match on a specific game state. hasGameStarted_ is 0 only during
// the boot load, so this never touches a normally-played match.
static inline void TestBoot_ApplyGameState(const BootStateConfig* c)
{
    if (!c->apply_state || inningSetting.rel != 5)
        return;

    if (g_GameLogic.EventTriggers_GameHasStarted == 0)
    {
        // during the load: everything EXCEPT the outs, re-applied every frame
        g_Scores.Inning     = c->inning;
        g_Scores.halfInning = c->bottom_of_inning;
        g_GameLogic.homeTeamBattingInd_fieldingTeam  = c->bottom_of_inning ? 1 : 0;
        g_GameLogic.awayTeamBattingInd_battingTeam = c->bottom_of_inning ? 0 : 1;

        g_Scores.scores[0].total       = c->score_away;
        g_Scores.scores[0].byInning[0] = c->score_away;  // book all runs in the 1st
        g_Scores.scores[1].total       = c->score_home;
        g_Scores.scores[1].byInning[0] = c->score_home;

        g_Strikes.balls   = c->balls;
        g_Strikes.strikes = c->strikes;

        g_GameLogic.TeamStars[0] = c->stars_p1;
        g_GameLogic.TeamStars[1] = c->stars_p2;
        g_GameLogic.IsStarChance = c->star_chance;

        // mapping slot 0 is the pitcher copy; slots 1-9 are the batting order
        if (c->apply_order)
        {
            for (int team = 0; team < 2; team++)
            {
                for (int i = 0; i < 9; i++)
                {
                    int pos = c->orderPos_A_H[team][i];
                    // [0] is the batter's inMemRoster index, which is i in this ordering
                    g_GameLogic.battingOrderAndPositionMapping[team][i + 1][0] = i;
                    g_GameLogic.battingOrderAndPositionMapping[team][i + 1][1] = pos;
                    if (pos == 0)
                    {
                        g_GameLogic.battingOrderAndPositionMapping[team][0][0] = i;
                        g_GameLogic.battingOrderAndPositionMapping[team][0][1] = 0;
                    }
                }
                g_GameLogic.currentBatterPerTeam[team] = c->currentBatter_A_H[team];
            }

            for (int team = 0; team < 2; team++)
            {
                for (int i = 0; i < 9; i++)
                {
                    inMemRoster[team][i].stats.FieldingArm    = c->orderFieldHand_P1P2[team][i];
                    inMemRoster[team][i].stats.BattingStance  = c->orderBatHand_P1P2[team][i];
                    inMemRoster[team][i].stats.UnusedBytes[0] = c->orderSuperstar_P1P2[team][i];
                }
            }
        }

        for (int i = 0; i < 3; i++)
        {
            if (c->runner_present[i])
            {
                tbm_runnerRosterLoc(i)  = c->runner_rosterLoc[i];
                tbm_runnerCharID(i)     = c->runner_charID[i];
                tbm_runnerClearInstr(i) = 0x60000000; // nop
            }
        }
        return;
    }

    // match started: put the runner-clear instructions back
    for (int i = 0; i < 3; i++)
    {
        if (c->runner_present[i])
            tbm_runnerClearInstr(i) = tbm_runnerRestoreInstr(i);
    }

    // outs go on a few seconds after the start (inline outs silence the BGM)
    if (tbm_outsCounter >= TBM_OUTS_DELAY_FRAMES + TBM_OUTS_BURST_FRAMES)
        return;                                  // done; hands off the match
    tbm_outsCounter++;
    if (tbm_outsCounter <= TBM_OUTS_DELAY_FRAMES)
        return;                                  // let the BGM get going first

    // never overwrite the outs once a pitch is in flight
    if (g_Stats.atBatPitchThrown != 0)
    {
        tbm_outsCounter = TBM_OUTS_DELAY_FRAMES + TBM_OUTS_BURST_FRAMES;  // give up
        return;
    }

    g_Strikes.outs       = c->outs;
    g_Strikes.storedOuts = c->outs;
}

// Per-frame entry: no .address, and no .state because it must run through the
// menu -> match transition.
CGECKO(TestNewBootMatch, .notes = "Boots directly into a predetermined match.");
void TestNewBootMatch()
{
    const BootMatchSpec*   s = &boot_payload.spec;
    const BootStateConfig* c = &boot_payload.config;
    OPAQUE_PTR(s);
    OPAQUE_PTR(c);

    if (inningSetting.rel == 4)
        TestBoot_StageMatch(s, c);

    TestBoot_ApplyGameState(c);
}


// Fix for duplicate characters (docs/duplicate_characters.md). No .instruction
// on either hook: both REPLACE the instruction they overwrite.
CGECKO(DupLoadBindCharID, .address = 0x800156B0);
void DupLoadBindCharID()
{
    READ_GAME_REG(u32, entry, 5);                  // r5 = &table[r9]
    u32 obj = *(u32*)entry;                        // table[r9]
    WRITE_GAME_REG(3, obj ? obj : 0x80370F1C);
}

CGECKO(DupLoadBindStore, .address = 0x800156E4);
void DupLoadBindStore()
{
    // READ_GAME_REG can't be used twice in one function; saved r<n> is at r30 + 0x8 + (n-3)*4
    register u32 _fp __asm__("r30");
    u32 r3    = *(volatile u32*)(_fp + 0x8);              // saved r3 = table[r9]
    u32 r8    = *(volatile u32*)(_fp + 0x8 + ((8 - 3) << 2)); // saved r8
    u32 model = *(u32*)(0x8036E548 + (r8 << 2) + 11456);  // model[r8]
    *(u32*)((r3 ? r3 : 0x80370F1C) + 24) = model;          // bind; scratch on NULL
}
