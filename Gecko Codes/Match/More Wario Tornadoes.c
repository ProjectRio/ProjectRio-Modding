/*###########################################################
# More Wario Tornadoes
###########################################################*/
// Author: Nuche17
// Why 5 is the limit and how a tornado catches and throws: docs/match_codes.md
#include "CGecko/Common.h"
#include "Include/types.h"
#include "Include/Symbols/game.h"

typedef struct PalaceTornadoPlacement
{
    /*0x00*/ f32 x;
    /*0x04*/ f32 y;            /* positive is up (bobs 0.5 m around it) */
    /*0x08*/ f32 z;
    /*0x0C*/ u8  usedFlag;     /* 3 = tornado, 13 = end of list */
    /*0x0D*/ u8  _0D;
    /*0x0E*/ u8  group;
    /*0x0F*/ u8  _0F;
    /*0x10*/ f32 restYaw;
    /*0x14*/ f32 scale[3];
    /*0x20*/ f32 rangeBaseA;   /* throw-out angle, degrees: 0 = toward CF, */
    /*0x24*/ f32 rangeSpanA;   /* +90 = toward 1B side, 180 = toward home.  */
    /*0x28*/ f32 rangeBaseB;   /* A or B is picked by the random spin       */
    /*0x2C*/ f32 rangeSpanB;   /* direction, 50/50.                         */
    /*0x30*/ f32 spinRate;     /* deg/frame; throw speed ~= 5 m * this     */
} PalaceTornadoPlacement;      /* size: 0x34 */

#define TORNADO_TABLE_SIZE 6
#define g_TornadoTable ARRAY_1D_ADDRESS(PalaceTornadoPlacement, TORNADO_TABLE_SIZE, TornadoPlacementConfig_ADDR)

#define TORNADO           3
#define END_OF_LIST       13
#define VANILLA_TORNADOES 2
#define MAX_TORNADOES     5        /* the loader stops counting at 5 */

#define TORNADO_Y         7.0f     /* vanilla height */
#define VANILLA_SPIN      4.0f
/* Faster spin = faster throw-out, so a ball sent toward the 5 m fence 10 m away
   drops ~0.5 m on the way instead of ~1.1 m (it can be caught as low as 5.5 m). */
#define GAP_SPIN          6.0f

/* 2B is at (0, 38.7) and the CF starts at (0, 76): 76% of the way is z = 67.05.
   The gap spots are 10 m in from the wall (pyrio Wario Palace wall segments,
   z = -0.923|x| + 115 there), on the 22.5-degree line from home. */
static const PalaceTornadoPlacement EXTRA_TORNADOES[] = {
    /* Between 2B and the CF: throws out toward either gap, like vanilla's. */
    { 0.0f, TORNADO_Y, 67.05f, TORNADO, 1, 2, 0, 0.0f, { 1.0f, 1.0f, 1.0f },
      15.0f, 45.0f,  -60.0f, 45.0f,  VANILLA_SPIN },
    /* Right-center gap: A over the fence (the wall faces ~43 degrees here),
       B back toward the infield (2B is at ~-143 degrees). */
    { 27.7f, TORNADO_Y, 75.8f, TORNADO, 1, 1, 0, 30.0f, { 1.0f, 1.0f, 1.0f },
      0.0f, 90.0f,  -190.0f, 90.0f,  GAP_SPIN },
    /* Left-center gap, mirrored. */
    { -27.7f, TORNADO_Y, 75.8f, TORNADO, 1, 3, 0, 330.0f, { 1.0f, 1.0f, 1.0f },
      -90.0f, 90.0f,  100.0f, 90.0f,  GAP_SPIN },
};
#define EXTRA_TORNADO_COUNT (sizeof(EXTRA_TORNADOES) / sizeof(EXTRA_TORNADOES[0]))
_Static_assert(VANILLA_TORNADOES + EXTRA_TORNADO_COUNT <= MAX_TORNADOES, "too many tornadoes");

/* loadWarioPalace+0x4 (`mflr r0`): runs on every Wario Palace load, after
   game.rel has been reloaded with a stock table and before the tornadoes are
   counted. The wrapper restores r3 (files), so the loader is unaffected. */
CGECKO(MoreWarioTornadoes, .address = 0x80715864, .state = MSSB_GAME,
       .instruction = "mflr r0",
       .notes = "Adds 3 tornadoes to Wario Palace: one between\n"
                "2nd base and center field, and one deep in each\n"
                "gap that throws the ball over the fence or back\n"
                "toward the infield.");
void MoreWarioTornadoes(void)
{
    /* Word copy: a 0x34-byte struct assignment becomes a memcpy call, which
       a gecko code has no copy of. */
    const u32* src = (const u32*)EXTRA_TORNADOES;
    volatile u32* dst = (volatile u32*)&g_TornadoTable[VANILLA_TORNADOES];

    for (u32 k = 0; k < sizeof(EXTRA_TORNADOES) / sizeof(u32); k++)
        dst[k] = src[k];

    g_TornadoTable[VANILLA_TORNADOES + EXTRA_TORNADO_COUNT].usedFlag = END_OF_LIST;
}
