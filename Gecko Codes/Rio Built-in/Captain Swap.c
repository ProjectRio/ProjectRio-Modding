/*###########################################################
# Captain Swap
###########################################################*/
// Author: nuche, LittleCoaks
// Hook site (characterSelectControls' Start-button branch) and the menus.rel state it edits: docs/captain_swap.md
#include "Include/static/UnknownHomes_Static.h"
#include "Include/Symbols/dol.h"
#include "Include/Symbols/menus.h"
#include "Include/Unknown/File_0x800678cc.h"
#include "Include/Unknown/File_0x800625a4.h"
#include "Include/Unknown/File_0x80034e20.h"
#include "Include/musyx/musyx.h"
#include "Include/Dolphin/pad.h"

// menus.rel character-select state (decomp: menuCursors / lbl_2_bss_F468, not yet typed)
#define CharSelectCursor(team)        VAR_ADDRESS(s32, 0x80750C48 + 4 * (team))
#define CharSelectOnBottomHalf(team)  VAR_ADDRESS(u8, 0x80750C89 + (team))
#define CharSelectShowingProfile(team) VAR_ADDRESS(u8, 0x80750C8D + (team))
#define ChemStarsArg                  VAR_ADDRESS(int, 0x803530EC)      // Static_Stats_Tables + 0x474C
#define MenuSoundVolumes              ((u8*)0x800EFBA4)                 // lbl_800EFBA4
#define CaptainBackground(team)       (((UIRecord*)0x8039C3E0)[176 + (team)].textureOverride[1])

#define CAPTAIN_CURSOR_SLOTS   9
#define CAPTAIN_LIST_LEN       12
#define SFX_CAPTAIN_SWAPPED    0x1BC
#define SFX_CANNOT_SWAP        0x1BA
#define TEXTURE_NO_CAPTAIN     0x3B

static const struct { u8 charID; u16 texture; } CAPTAIN_BACKGROUNDS[] = {
    { CHAR_ID_MARIO,    0x4A }, { CHAR_ID_LUIGI,   0x5B }, { CHAR_ID_PEACH,   0x56 },
    { CHAR_ID_YOSHI,    0x58 }, { CHAR_ID_DK,      0x59 }, { CHAR_ID_BOWSER,  0x5A },
    { CHAR_ID_DAISY,    0x5C }, { CHAR_ID_BIRDO,   0x5E }, { CHAR_ID_WALUIGI, 0x5D },
    { CHAR_ID_DIDDY,    0x5F }, { CHAR_ID_WARIO,   0x57 }, { CHAR_ID_BOWSERJR, 0x60 },
};

static u16 BackgroundFor(u8 charID)
{
    for (int i = 0; i < (int)LEN(CAPTAIN_BACKGROUNDS); i++)
        if (CAPTAIN_BACKGROUNDS[i].charID == charID)
            return CAPTAIN_BACKGROUNDS[i].texture;
    return TEXTURE_NO_CAPTAIN;
}

static bool IsAllowedCaptain(u8 charID)
{
    const u8* captains = (const u8*)captainIDMappings_ADDR;
    for (int i = 0; i < CAPTAIN_LIST_LEN; i++)
        if (captains[i] == charID)
            return true;
    return false;
}

static void PlayMenuSound(u16 fx)
{
    sndFXStartEx(fx, MenuSoundVolumes[3], 0x3F, 0);
}

static bool SwapCaptain(int team)
{
    structCharSelect* roster = &cursorPositions.roster;

    if (CharSelectShowingProfile(team) || CharSelectOnBottomHalf(team))
        return false;

    s32 cursor = CharSelectCursor(team);
    if (cursor >= CAPTAIN_CURSOR_SLOTS)
        return false;

    int slot = 0;
    while (slot < CAPTAIN_CURSOR_SLOTS && roster->positionSwapMapping[team][slot] != cursor)
        slot++;
    if (slot == CAPTAIN_CURSOR_SLOTS)
        return false;

    u8 newCaptain = roster->rosterCharID[team][slot];
    if (!IsAllowedCaptain(newCaptain))
        return false;

    Static_Stats_Tables.captainSelectedID[team] = newCaptain;

    u8 oldCaptain = roster->rosterCharID[team][0];
    roster->rosterCharID[team][0] = newCaptain;
    roster->rosterCharID[team][slot] = oldCaptain;

    CaptainBackground(team) = BackgroundFor(newCaptain);

    u8 oldMapping = roster->positionSwapMapping[team][0];
    roster->positionSwapMapping[team][0] = cursor;
    roster->positionSwapMapping[team][slot] = oldMapping;

    const u8* chemistry = (const u8*)&Static_Stats_Tables.characterStats[newCaptain].chemistry;
    for (int i = 1; i < CAPTAIN_CURSOR_SLOTS; i++)
    {
        u8 charID = roster->rosterCharID[team][i];
        if (charID != 0xFF)
            roster->chemWCaptain[team][i] = chemistry[charID];
    }

    teamLogoDetermination(team);
    FUNCTION_ADDRESS(void, teamSelectionSetChemStars_ADDR, int, int)(ChemStarsArg, team);
    updateCharacterSelectProcessCode(team, 0xF);
    return true;
}

// Replaces the `bne` into the game's own Start handling, so Start swaps captains instead.
CGECKO(CaptainSwap, .address = 0x8064F67C, .state = MSSB_MENU,
       .notes = "On the team select screen, press Start over a character to make them the captain.");
void CaptainSwap(void)
{
    register unsigned int _sp __asm__("r30");
    u32 newInput = *(volatile u32*)(_sp + 0x8 + ((30 - 3) << 2));
    int team     = *(volatile int*)(_sp + 0x8 + ((27 - 3) << 2));

    if (!(newInput & PAD_BUTTON_START))
        return;

    PlayMenuSound(SwapCaptain(team) ? SFX_CAPTAIN_SWAPPED : SFX_CANNOT_SWAP);
}
