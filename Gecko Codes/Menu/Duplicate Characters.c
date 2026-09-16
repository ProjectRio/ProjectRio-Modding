/*###########################################################
# Duplicate Characters
###########################################################*/
// Author: LittleCoaks

// *Lets the same character be drafted onto both teams, as many times as you
// *want. Converted from the community gecko code "All Duplicate Characters".
// *Duplicates and colour variants (all Toads, all Shy Guys, ...) get max
// *chemistry with each other, so a stacked team is not a chemistry-less one.
//
// HOW THE GAME TRACKS IT. Character select keeps a 54-byte "already taken"
// table, one byte per character id, at Static_Stats_Tables + 0x4757
// (0x803530F7). A non-zero byte greys that character out. The stock code sets
// it from five `stb r5, 0x4757(r3)` sites in addRemoveCharVariantRelated
// (0x80067B40) plus their menus.rel counterparts.
//
// The mod is therefore: stop the game marking anyone taken, clear whatever is
// already marked, and then put the mark back for JUST the two captains -- they
// are genuinely unavailable, the rest are not.
//
// WHY THIS IS A PER-FRAME CODE AND NOT A PILE OF 04 WRITES. The original is a
// static patch list, which is fine for a code you enable in an ini and forget.
// This one is a TOGGLE, so it has to be able to put the game back -- and a REL
// patch has to be re-applied every time menus.rel reloads anyway.
//
// Original gecko code, for reference:
//     003530f7 00350000   zero 54 bytes at 0x803530F7   (0x35+1, the count is
//     003c6050 002300ff   36 bytes of 0xFF at 0x803C6050  the HIGH halfword --
//                                                         see codehandler.s
//                                                         `rlwinm r10,r4,16,16,31`)
//     04067bac/bc8/be4/c00/c1c 60000000   nop the five stb sites (DOL)
//     0464ec28/ec38/ece8       60000000   the same three, menus.rel
//     0404e548 60000000  046553f8 60000000
//     0404e6b0 2C0000FF  046553d4 2C0000FF
//     c264f394 ...       re-mark the two captains
#include "Include/game/UnknownHomes_Game.h"
#include "Include/static/UnknownHomes_Static.h"

#define TAKEN_TABLE   0x803530F7    // Static_Stats_Tables + 0x4757, 54 bytes
#define TAKEN_COUNT   54            // 0x35 + 1, one byte per character id
#define CHARSEL_SLOTS 0x803C6050    // charSelectStruct + 0x28, 36 bytes
#define CHARSEL_COUNT 36            // 0x23 + 1
#define CAPTAIN_A     0x803C6726    // cursorPositions + 0x02
#define CAPTAIN_B     0x803C672F    // cursorPositions + 0x0B

// menuCtrl->screenCode, and the two screens this code has to tell apart.
// screenFuncTable: 9 = captainSelect (0x8065201C), 10 = teamSelect
// (0x80650144), the character draft.
#define MENU_SCREEN   (*(u16*)(*(u32*)0x803CBBCC + 2))
#define SCREEN_CAPTAIN_SELECT 9
#define SCREEN_TEAM_SELECT    10

#define NOP           0x60000000
#define CMPWI_R0_FF   0x2C0000FF    // cmpwi r0, 0xFF   (0xFF = "empty slot")

// (site, original, patched). Patching is Common.h's PatchInstruction_Conditional,
// which only writes when the expected word is there -- so ON and OFF are the same
// call with the last two arguments swapped, and both are idempotent. It also
// re-arms itself for free: after a menus.rel reload the original is back, the
// site matches again, and the next frame re-applies. No saved-state RAM needed.
//
// The REL originals are NOT the same instructions as their DOL counterparts
// (stwx vs sth, cmpw r3,r0 vs cmpw r0,r7) -- read out of live RAM with menus.rel
// resident, not assumed.
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
    // CGECKO_ACTIVE is 1 on its own; a pack that wants to toggle this mod
    // defines it to a runtime condition before including the file. This file
    // never knows where that condition lives.
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

    // Clear every "taken" mark, then put back only the two captains -- they
    // really are unavailable. This is the C2 the original code injected at
    // 0x8064F394, hoisted to per-frame: the table is only read while the
    // character-select screen is drawing, so refreshing it each frame is
    // equivalent and needs no second injection site.
    for (i = 0; i < TAKEN_COUNT; i++)
        VAR_ADDRESS(u8, TAKEN_TABLE + i) = 0;
    for (i = 0; i < CHARSEL_COUNT; i++)
        VAR_ADDRESS(u8, CHARSEL_SLOTS + i) = 0xFF;

    // Re-mark the two captains ONLY while the draft is on screen.
    //
    // THE BUG THIS FIXES. cursorPositions is not cleared between visits, so on
    // the CAPTAIN-select screen it still holds the captains chosen last time
    // round. Marking those two here greys out exactly the characters the user
    // came back to pick again -- reported as "I cannot select my previous
    // captains when I reenter the captain select screen". The marks were ours,
    // not the game's: with this code off, the same trip works.
    //
    // The wipes above stay unconditional. Clearing the taken table on any other
    // menu screen is harmless and correct -- with duplicates on, nobody is
    // taken until the draft says so -- and it is what makes the captain screen
    // behave again. It is only the re-marking that is draft-specific.
    if (MENU_SCREEN != SCREEN_TEAM_SELECT)
        return;

    // A cursor slot reads 0xFF until a captain has been picked, and
    // TAKEN_TABLE + 0xFF is 200 bytes past the end of a 54-byte table -- a
    // stray write into Static_Stats_Tables. Only ever mark a real character id.
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

   Static_Stats_Tables.characterStats is the master stat table, one row per
   CHAR_ID; rows are copied into inMemRoster when a roster is built, so this
   is the one place to edit chemistry for everyone. Every character gets max
   chemistry with a copy of themselves, and every colour variant with the other
   members of its group. Per frame, like the 00-type byte writes it replaced,
   so it never matters when the game (re)loads the table -- and it runs in
   every rel state because the table is read during the match too.
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
    // Same toggle as the draft patches above. Nothing to undo when OFF: the
    // stock values come back with the next table load from disc.
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
