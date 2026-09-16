/*###########################################################
# Debug Mode From Main Menu
###########################################################*/
// Author: LittleCoaks
// Minimal loader only: no text overlay, no in-suite fixes (those are in
// Gecko Codes/Global/Debug Mode.c). Mechanics: docs/debug_rel.md, docs/rel_loader.md.

// DOL loader node (lbl_80111300)
#define LOADER_FINISHED   VAR_ADDRESS(short,    0x80111310)
#define LOADER_STATE      VAR_ADDRESS(halfword, 0x80111318)
#define LOADER_STATE_LOAD_GAME    0xB
#define LOADER_STATE_RELOAD_MENU  0x8

// REL file table, menu entry (0x800E8AA8): words 1..3
#define RELTAB_MENU_SIZE   VAR_ADDRESS(word, 0x800E8AAC)   // flag | decompSize
#define RELTAB_MENU_OFFSET VAR_ADDRESS(word, 0x800E8AB0)
#define RELTAB_MENU_CSIZE  VAR_ADDRESS(word, 0x800E8AB4)
#define DEBUG_REL_SIZE     0x4005912C
#define DEBUG_REL_OFFSET   0x00150000
#define DEBUG_REL_CSIZE    0x000271C0

// Project Rio scene id (0 boot / 4 menus.rel / 5 game.rel)
#define RIO_SCENE_ID      VAR_ADDRESS(halfword, 0x800E877C)

#define removeCurrentDrawingItem  FUNCTION_ADDRESS(void, 0x800B0A14, void)
#define text_freeAllBlocks        FUNCTION_ADDRESS(void, 0x8000FE54, void)
#define game_memset               FUNCTION_ADDRESS(void, 0x8000540C, void*, int, unsigned int)

// The DOL's 2D element pool (menuGraphicsStructures): 864 records of 0xC0.
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
    RELTAB_MENU_SIZE   = DEBUG_REL_SIZE;
    RELTAB_MENU_OFFSET = DEBUG_REL_OFFSET;
    RELTAB_MENU_CSIZE  = DEBUG_REL_CSIZE;

    g_debugArm      = DEBUG_ARMED;
    LOADER_FINISHED = 1;              // loader: tear menus.rel down next frame
    removeCurrentDrawingItem();       // and stop running the menu, as stock does
    RIO_SCENE_ID    = 0;              // keep menu-gated codes out of the slot debug.rel takes
}

// 2. Loader: the only place state 0xA advances to 0xB. Write 8 (reload the
//    menu slot, now debug.rel) when a launch is armed, 0xB otherwise.
CGECKO(loader_reloads_menu_slot, .address = 0x80009C0C, .state = MSSB_ALWAYS,
                                 .instruction = "nop");
void loader_reloads_menu_slot(void)
{
    if (g_debugArm == DEBUG_ARMED)
    {
        g_debugArm   = DEBUG_LOADING;
        LOADER_STATE = LOADER_STATE_RELOAD_MENU;
    }
    else
        LOADER_STATE = LOADER_STATE_LOAD_GAME;
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
    game_memset(UI_ELEMENT_POOL, 0, UI_ELEMENT_POOL_SIZE);  // menu sprites + fade
    text_freeAllBlocks();                                    // menu text blocks
}
