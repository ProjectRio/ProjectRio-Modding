#ifndef RIO_GAME_SETTINGS_DEFAULTS_H
#define RIO_GAME_SETTINGS_DEFAULTS_H
// The defaults block that fills gameSettings: docs/import_menu.md
#include "Include/types.h"

#define GAMESETTINGS_INNINGS  VAR_ADDRESS(u8, 0x803C5F04 + 0x3E)
#define GAMESETTINGS_MERCY    VAR_ADDRESS(u8, 0x803C5F04 + 0x3F)

enum { INNINGS_1, INNINGS_3, INNINGS_5, INNINGS_7, INNINGS_9 };

#define DEFAULT_INNINGS_LOAD  0x800498C4   // li r0, INNINGS_5

static inline void GameSettings_SetDefaultInnings(u32 inningsIndex)
{
    PatchInstruction(DEFAULT_INNINGS_LOAD, 0x38000000 | inningsIndex);
}

#endif
