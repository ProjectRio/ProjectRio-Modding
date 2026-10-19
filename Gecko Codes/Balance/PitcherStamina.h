// The per-pitcher stamina counter and how initializeStats seeds it: docs/import_balance.md
#pragma once
#include "Include/static/UnknownHomes_Static.h"

#define PITCHER_STATS_BASE     0x803535C8   /* Static_Stats_Tables + 0x4C28, [2 teams][9 players] */
#define PITCHER_STATS_SIZE     0x1E
#define PITCHER_STAMINA_OFFSET 0x10         /* s16 */
#define PITCHER_STATS_COUNT    18

#define FULL_PITCHER_STAMINA   10

#define INIT_STATS_LOAD_STAMINA 0x806BA3DC   /* initializeStats: clrlwi r7, r3, 16 */
#define CLRLWI_R7_R3_16         0x5467043E
#define LI_R7(value)            (0x38E00000 | (value))
