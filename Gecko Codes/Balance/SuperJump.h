// jumpArray[hasSuperJump] = { launch speed, gravity, drift, frames }: docs/import_balance.md
#pragma once
#include "Include/mssbTypes.h"
#include "Include/Symbols/game.h"

#define SUPER_DUPER_JUMP_GRAVITY  0x3BA3D70A   /* 0.005f, stock 0.02f */

#define jumpArrayWords ARRAY_2D_ADDRESS(u32, 2, 4, jumpArray_ADDR)

static inline void SetSuperJumpGravity(u32 gravityBits)
{
    jumpArrayWords[1][1] = gravityBits;
}
