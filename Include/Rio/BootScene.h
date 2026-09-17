#ifndef RIO_BOOT_SCENE_H
#define RIO_BOOT_SCENE_H
// The boot walk's scene dispatch and what each boot code does to it: docs/import_menu.md
#include "Include/static/UnknownHomes_Static.h"

#define BOOT_SCENE_DISPATCH      0x8063F964
#define BOOT_SCENE_LI_R3(scene)  (0x38600000 | (scene))

#define BOOT_REL                 0
#define SCENE_GAME_MODE_LOAD     4
#define SCENE_MAIN_MENU          5
#define SCENE_PROGRESSIVE_PROMPT 0x11

static inline void BootScene_Force(u32 scene)
{
    if (inningSetting.rel == BOOT_REL)
        PatchInstruction(BOOT_SCENE_DISPATCH, BOOT_SCENE_LI_R3(scene));
}

// Body of a hook AT the dispatch (no .instruction): r3 carries the scene.
static inline void BootScene_EnterGameMode(u8 gameType)
{
    g_d_GameSettings.GameModeSelected = gameType;
    WRITE_GAME_REG(3, SCENE_GAME_MODE_LOAD);
}

#endif
