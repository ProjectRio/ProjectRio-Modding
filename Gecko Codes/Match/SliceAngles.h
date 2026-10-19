#ifndef SLICE_ANGLES_H
#define SLICE_ANGLES_H
// Table layout and what a slice is: docs/import_match_gameplay.md
#include "Include/game/UnknownHomes_Game.h"
#include "Include/Symbols/game.h"

/* [stick direction][charge or star][swing frame][lower, upper] */
#define BattingAngleRanges ARRAY_4D_ADDRESS(s16, 3, 2, 15, 2, BattingAngleRanges_ADDR)

typedef struct { u8 direction, charged, frame; s16 angle; } SliceAngle;

static inline BOOL SliceAnglesUntouched(void)
{
    return BattingAngleRanges[0][1][2][0] == 0 && BattingAngleRanges[0][1][2][1] == 0;
}

static inline void ApplySliceAngles(const SliceAngle* fouls, int count)
{
    int i;
    for (i = 0; i < count; i++)
    {
        s16* range = BattingAngleRanges[fouls[i].direction][fouls[i].charged][fouls[i].frame];
        range[0] = fouls[i].angle;
        range[1] = fouls[i].angle;
    }
}

#endif
