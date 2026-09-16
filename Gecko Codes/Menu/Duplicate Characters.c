/*###########################################################
# Duplicate Characters
###########################################################*/
// Author: LittleCoaks
// Converted from the community gecko code "All Duplicate Characters".
// The taken table, patch sites and the captain re-mark bug: docs/duplicate_characters.md
#include "Include/game/UnknownHomes_Game.h"
#include "Include/static/UnknownHomes_Static.h"

#define TAKEN_TABLE   0x803530F7    // Static_Stats_Tables + 0x4757, 54 bytes
#define TAKEN_COUNT   54            // 0x35 + 1, one byte per character id
#define CHARSEL_SLOTS 0x803C6050    // charSelectStruct + 0x28, 36 bytes
#define CHARSEL_COUNT 36            // 0x23 + 1
#define CAPTAIN_A     0x803C6726    // cursorPositions + 0x02
#define CAPTAIN_B     0x803C672F    // cursorPositions + 0x0B

// menuCtrl->screenCode: 9 = captainSelect, 10 = teamSelect (the draft)
#define MENU_SCREEN   (*(u16*)(*(u32*)0x803CBBCC + 2))
#define SCREEN_CAPTAIN_SELECT 9
#define SCREEN_TEAM_SELECT    10

#define NOP           0x60000000
#define CMPWI_R0_FF   0x2C0000FF    // cmpwi r0, 0xFF   (0xFF = "empty slot")

// (site, original, patched); the REL originals differ from their DOL counterparts
typedef struct { u32 addr; u32 orig; u32 patched; } CodePatch;

static const CodePatch PATCHES[] = {
    /* --- main.dol: addRemoveCharVariantRelated, five stb sites -------- */
    { 0x80067BAC, 0x98A34757, NOP },          /* stb r5, 0x4757(r3) */
    { 0x80067BC8, 0x98A34757, NOP },
    { 0x80067BE4, 0x98A34757, NOP },
    { 0x80067C00, 0x98A34757, NOP },
    { 0x80067C1C, 0x98A34757, NOP },
    { 0x8004E548, 0xB0080018, NOP },          /* sth r0, 0x18(r8) */
    { 0x8004E6B0, 0x7C003800, CMPWI_R0_FF },  /* cmpw r0, r7 */
    /* --- menus.rel: the same edits on the menu side ------------------- */
    { 0x8064EC28, 0x98E44757, NOP },          /* stb r7, 0x4757(r4) */
    { 0x8064EC38, 0x98E54757, NOP },          /* stb r7, 0x4757(r5) */
    { 0x8064ECE8, 0x98A34757, NOP },          /* stb r5, 0x4757(r3) */
    { 0x806553F8, 0x7C1EF92E, NOP },          /* stwx r0, r30, r31 */
    { 0x806553D4, 0x7C030000, CMPWI_R0_FF },  /* cmpw r3, r0 */
};
#define N_PATCHES ((int)(sizeof(PATCHES) / sizeof(PATCHES[0])))

CGECKO(DuplicateCharacters, .state = MSSB_MENU,
       .notes = "Allows you to draft any character as many times as you want.\n"
                "Duplicates and colour variants have max chemistry.");
void DuplicateCharacters()
{
    // a pack may define CGECKO_ACTIVE to a runtime condition before including this file
    bool on = CGECKO_ACTIVE;
    int i;
    u8  cap;

    for (i = 0; i < N_PATCHES; i++)
    {
        if (on)
            PatchInstruction_Conditional(PATCHES[i].addr, PATCHES[i].orig,
                                         PATCHES[i].patched);
        else
            PatchInstruction_Conditional(PATCHES[i].addr, PATCHES[i].patched,
                                         PATCHES[i].orig);
    }

    if (!on)
        return;

    // clear every "taken" mark (per frame; the table is only read while character select draws)
    for (i = 0; i < TAKEN_COUNT; i++)
        VAR_ADDRESS(u8, TAKEN_TABLE + i) = 0;
    for (i = 0; i < CHARSEL_COUNT; i++)
        VAR_ADDRESS(u8, CHARSEL_SLOTS + i) = 0xFF;

    // re-mark the two captains ONLY on the draft; on the captain screen the cursor holds last time's picks
    if (MENU_SCREEN != SCREEN_TEAM_SELECT)
        return;

    // a cursor slot reads 0xFF until a captain has been picked; only ever mark a real id
    cap = VAR_ADDRESS(u8, CAPTAIN_A);
    if (cap < TAKEN_COUNT)
        VAR_ADDRESS(u8, TAKEN_TABLE + cap) = 1;

    cap = VAR_ADDRESS(u8, CAPTAIN_B);
    if (cap < TAKEN_COUNT)
        VAR_ADDRESS(u8, TAKEN_TABLE + cap) = 1;
}

/* =========================================================================
   Duplicates & variants have chemistry
   (Authors: PeacockSlayer, LittleCoaks -- was the standalone code of that name)
   ========================================================================= */
#define MAX_CHEMISTRY 99
#define END -1

static const s8 kVariantGroups[][6] = {
    { CHAR_ID_KOOPA_GREEN,     CHAR_ID_KOOPA_RED,       END },
    { CHAR_ID_PARATROOPA_RED,  CHAR_ID_PARATROOPA_GREEN, END },
    { CHAR_ID_TOAD_RED,        CHAR_ID_TOAD_BLUE,       CHAR_ID_TOAD_YELLOW,  CHAR_ID_TOAD_GREEN,  CHAR_ID_TOAD_PURPLE, END },
    { CHAR_ID_SHYGUY_RED,      CHAR_ID_SHYGUY_BLUE,     CHAR_ID_SHYGUY_YELLOW, CHAR_ID_SHYGUY_GREEN, CHAR_ID_SHYGUY_BLACK, END },
    { CHAR_ID_PIANTA_BLUE,     CHAR_ID_PIANTA_RED,      CHAR_ID_PIANTA_YELLOW, END },
    { CHAR_ID_NOKI_BLUE,       CHAR_ID_NOKI_RED,        CHAR_ID_NOKI_GREEN,   END },
    { CHAR_ID_BRO_HAMMER,      CHAR_ID_BRO_FIRE,        CHAR_ID_BRO_BOOMERANG, END },
    { CHAR_ID_MAGIKOOPA_BLUE,  CHAR_ID_MAGIKOOPA_RED,   CHAR_ID_MAGIKOOPA_GREEN, CHAR_ID_MAGIKOOPA_YELLOW, END },
    { CHAR_ID_DRYBONES_GRAY,   CHAR_ID_DRYBONES_GREEN,  CHAR_ID_DRYBONES_RED, CHAR_ID_DRYBONES_BLUE, END },
};

CGECKO(DuplicatesHaveChemistry);
void DuplicatesHaveChemistry(void)
{
    // nothing to undo when OFF: the stock values come back with the next table load
    if (!CGECKO_ACTIVE)
        return;

    // ChemistryTable is one u8 per CHAR_ID, in id order
    #define CHEM(me, other) ((u8*)&Static_Stats_Tables.characterStats[me].chemistry)[other]

    // everyone with a copy of themselves
    for (int id = 0; id < NUM_CHOOSABLE_CHARACTERS; id++)
        CHEM(id, id) = MAX_CHEMISTRY;

    // every variant with every other variant in its group
    for (int g = 0; g < (int)(sizeof(kVariantGroups) / sizeof(kVariantGroups[0])); g++)
        for (const s8* a = kVariantGroups[g]; *a != END; a++)
            for (const s8* b = kVariantGroups[g]; *b != END; b++)
                CHEM(*a, *b) = MAX_CHEMISTRY;
}
