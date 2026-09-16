/*###########################################################
# Debug Mode From Main Menu
###########################################################*/
// Author: LittleCoaks
// Minimal loader only: no text overlay, no in-suite fixes (those are in
// Gecko Codes/Global/Debug Mode.c). Mechanics: docs/debug_rel.md, docs/rel_loader.md.

#include "Include/game/UnknownHomes_Game.h"
#include "Include/static/UnknownHomes_Static.h"   // inningSetting.rel: the Rio scene id
#include "Include/Unknown/File_0x800b0a14.h"
#include "Include/text/text_block.h"
#include "Include/Dolphin/stl.h"
#include "Include/Rio/RelTable.h"

// The DOL's 2D element pool (menuGraphicsStructures, no _ADDR in the decomp yet): 864 records of 0xC0.
#define UI_ELEMENT_POOL           ((void*)0x8039C3E0)
#define UI_ELEMENT_POOL_SIZE      0x28800

// Claimed RAM: "a debug launch is in flight" latch.
#define g_debugArm        VAR_ADDRESS(word, 0x802EC2B4)
#define DEBUG_ARMED       0x0DEB0601
#define DEBUG_LOADING     0x0DEB0602

// 1. Records button: drop the `bl changeScreenVariables(8)` and hand the menu
//    back to the loader instead.
CGECKO(records_launches_debug, .address = 0x8064184C, .state = MSSB_MENU,
                               .instruction = "nop",
                               .notes = "Replaces the Records button on the main menu with\n"
                                        "the game's hidden developer debug menu.");
void records_launches_debug(void)
{
    RelTable_PointMenuSlotAtDebugRel();

    g_debugArm         = DEBUG_ARMED;
    RelLoader_Finished = 1;           // loader: tear menus.rel down next frame
    removeCurrentDrawingItem();       // and stop running the menu, as stock does
    inningSetting.rel  = 0;           // keep menu-gated codes out of the slot debug.rel takes
}

// 2. Loader: the only place state 0xA advances to 0xB. Write 8 (reload the
//    menu slot, now debug.rel) when a launch is armed, 0xB otherwise.
CGECKO(loader_reloads_menu_slot, .address = 0x80009C0C, .state = MSSB_ALWAYS,
                                 .instruction = "nop");
void loader_reloads_menu_slot(void)
{
    if (g_debugArm == DEBUG_ARMED)
    {
        g_debugArm      = DEBUG_LOADING;
        RelLoader_State = RELLOADER_STATE_RELOAD_MENU;
    }
    else
        RelLoader_State = RELLOADER_STATE_LOAD_GAME;
}

// 3. Loader state 9, right after the linked REL's prolog: wipe the menu's 2D
//    pool and text blocks. Doing this in the teardown frame crashes.
CGECKO(debug_rel_linked, .address = 0x80009BA4, .state = MSSB_ALWAYS,
                         .instruction = "li r0, 0");
void debug_rel_linked(void)
{
    if (g_debugArm != DEBUG_LOADING)
        return;
    g_debugArm = 0;
    memset(UI_ELEMENT_POOL, 0, UI_ELEMENT_POOL_SIZE);       // menu sprites + fade
    text_freeAllBlocks();                                    // menu text blocks
}
