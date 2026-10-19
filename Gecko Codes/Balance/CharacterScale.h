// Define CHARACTER_SCALE before including. Stock sizes and the integer copy: docs/import_balance.md
#pragma once
#include "Include/game/UnknownHomes_Game.h"

#define SCALED(stockSize) ((f32)((stockSize) * CHARACTER_SCALE))

static const f32 SCALED_CHARACTER_SIZES[NUM_CHOOSABLE_CHARACTERS] = {
    [CHAR_ID_MARIO]           = SCALED(1.18),
    [CHAR_ID_LUIGI]           = SCALED(1.18),
    [CHAR_ID_DK]              = SCALED(1.11),
    [CHAR_ID_DIDDY]           = SCALED(1.18),
    [CHAR_ID_PEACH]           = SCALED(1.18),
    [CHAR_ID_DAISY]           = SCALED(1.18),
    [CHAR_ID_YOSHI]           = SCALED(1.18),
    [CHAR_ID_BABYMARIO]       = SCALED(1.20),
    [CHAR_ID_BABYLUIGI]       = SCALED(1.20),
    [CHAR_ID_BOWSER]          = SCALED(1.00),
    [CHAR_ID_WARIO]           = SCALED(1.18),
    [CHAR_ID_WALUIGI]         = SCALED(1.00),
    [CHAR_ID_KOOPA_GREEN]     = SCALED(1.18),
    [CHAR_ID_TOAD_RED]        = SCALED(1.40),
    [CHAR_ID_BOO]             = SCALED(1.20),
    [CHAR_ID_TOADETTE]        = SCALED(1.40),
    [CHAR_ID_SHYGUY_RED]      = SCALED(1.20),
    [CHAR_ID_BIRDO]           = SCALED(1.18),
    [CHAR_ID_MONTY]           = SCALED(1.00),
    [CHAR_ID_BOWSERJR]        = SCALED(1.20),
    [CHAR_ID_PARATROOPA_RED]  = SCALED(1.20),
    [CHAR_ID_PIANTA_BLUE]     = SCALED(1.20),
    [CHAR_ID_PIANTA_RED]      = SCALED(1.20),
    [CHAR_ID_PIANTA_YELLOW]   = SCALED(1.20),
    [CHAR_ID_NOKI_BLUE]       = SCALED(1.20),
    [CHAR_ID_NOKI_RED]        = SCALED(1.20),
    [CHAR_ID_NOKI_GREEN]      = SCALED(1.20),
    [CHAR_ID_BRO_HAMMER]      = SCALED(1.20),
    [CHAR_ID_TOADSWORTH]      = SCALED(1.40),
    [CHAR_ID_TOAD_BLUE]       = SCALED(1.40),
    [CHAR_ID_TOAD_YELLOW]     = SCALED(1.40),
    [CHAR_ID_TOAD_GREEN]      = SCALED(1.40),
    [CHAR_ID_TOAD_PURPLE]     = SCALED(1.40),
    [CHAR_ID_MAGIKOOPA_BLUE]  = SCALED(1.20),
    [CHAR_ID_MAGIKOOPA_RED]   = SCALED(1.20),
    [CHAR_ID_MAGIKOOPA_GREEN] = SCALED(1.20),
    [CHAR_ID_MAGIKOOPA_YELLOW] = SCALED(1.20),
    [CHAR_ID_KINGBOO]         = SCALED(1.20),
    [CHAR_ID_PETEY]           = SCALED(1.10),
    [CHAR_ID_DIXIE]           = SCALED(1.18),
    [CHAR_ID_GOOMBA]          = SCALED(1.20),
    [CHAR_ID_PARAGOOMBA]      = SCALED(1.20),
    [CHAR_ID_KOOPA_RED]       = SCALED(1.18),
    [CHAR_ID_PARATROOPA_GREEN] = SCALED(1.20),
    [CHAR_ID_SHYGUY_BLUE]     = SCALED(1.20),
    [CHAR_ID_SHYGUY_YELLOW]   = SCALED(1.20),
    [CHAR_ID_SHYGUY_GREEN]    = SCALED(1.20),
    [CHAR_ID_SHYGUY_BLACK]    = SCALED(1.20),
    [CHAR_ID_DRYBONES_GRAY]   = SCALED(1.20),
    [CHAR_ID_DRYBONES_GREEN]  = SCALED(1.20),
    [CHAR_ID_DRYBONES_RED]    = SCALED(1.20),
    [CHAR_ID_DRYBONES_BLUE]   = SCALED(1.20),
    [CHAR_ID_BRO_FIRE]        = SCALED(1.20),
    [CHAR_ID_BRO_BOOMERANG]   = SCALED(1.20),
};

static inline void ApplyCharacterScale(void)
{
    const u32* scaledBits = (const u32*)SCALED_CHARACTER_SIZES;
    for (int id = 0; id < NUM_CHOOSABLE_CHARACTERS; id++)
        *(volatile u32*)&charSizeMultipliers[id][0] = scaledBits[id];
}
