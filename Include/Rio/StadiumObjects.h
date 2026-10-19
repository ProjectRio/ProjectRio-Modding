/*###########################################################
# StadiumObjects.h -- placement tables of stadium objects
###########################################################*/
// Layouts worked out for the imported stadium codes (docs/import_match_rules.md).
// Not in the decomp yet; this header goes away once they are upstreamed.

#ifndef RIO_STADIUMOBJECTS_H
#define RIO_STADIUMOBJECTS_H

#include "CGecko/Common.h"
#include "Include/types.h"

typedef struct StadiumObjectPlacement
{
    /*0x00*/ f32 x;
    /*0x04*/ f32 y;          /* negative is up */
    /*0x08*/ f32 z;
    /*0x0C*/ f32 rotation;
    /*0x10*/ u8  type;
    /*0x11*/ u8  _11[3];
} StadiumObjectPlacement;    /* size: 0x14 */

/* game.rel .data: thwompStaticValues */
#define BOWSER_THWOMP_COUNT 6
#define g_ThwompPlacements ARRAY_1D_ADDRESS(StadiumObjectPlacement, BOWSER_THWOMP_COUNT, 0x807C8B14)

/* game.rel .data: blocks */
#define PEACH_BLOCK_COUNT 16
#define g_PeachBlocks ARRAY_1D_ADDRESS(StadiumObjectPlacement, PEACH_BLOCK_COUNT, 0x807CD098)

enum
{
    PEACH_BLOCK_BRICK   = 0,
    PEACH_BLOCK_METAL   = 1,
    PEACH_BLOCK_NOTE    = 2,
    PEACH_BLOCK_OUTLINE = 3,
};

static inline void Thwomps_SetHeight(f32 y)
{
    for (int i = 0; i < BOWSER_THWOMP_COUNT; i++)
        g_ThwompPlacements[i].y = y;
}

static inline void PeachBlocks_SetType(u8 type)
{
    for (int i = 0; i < PEACH_BLOCK_COUNT; i++)
        g_PeachBlocks[i].type = type;
}

#endif
