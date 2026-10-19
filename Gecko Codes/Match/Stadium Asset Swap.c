/*###########################################################
# Stadium Asset Swap
###########################################################*/
// Author: LittleCoaks
// How the game picks a stadium file, and the stock table: docs/stadium_files.md
#include "Include/mssbTypes.h"                    // STADIUM_ID
#include "Include/Symbols/dol.h"                  // StadiumFiles_ADDR
#include "Include/Unknown/File_0x800a64e0.h"      // AssetLoadInstructions

#define StadiumFiles ((AssetLoadInstructions *)StadiumFiles_ADDR)
#define STADIUM_VARIANTS 3

/* The stock table, dumped from main.dol 0x800EFBE8; raw word 2 in the comment. */
/*                                  const       { flag, _, origSize }   diskLoc      compSize   */
#define ASSET_MARIO_MG0  { 0x0000040B, { 1, 0, 0x168A6C }, 0x06CFD000, 0x000C69A8 }  /* 40168A6C */
#define ASSET_MARIO_MG1  { 0x0000040B, { 1, 0, 0x1331E0 }, 0x06DC4000, 0x000B67C4 }  /* 401331E0 */
#define ASSET_MARIO_MG2  { 0x0000040B, { 1, 0, 0x11924C }, 0x06E7A800, 0x000A5764 }  /* 4011924C */
#define ASSET_BOWSER_MG0 { 0x0000040B, { 1, 0, 0x0FBAE0 }, 0x06F20000, 0x000BE038 }  /* 400FBAE0 */
#define ASSET_BOWSER_MG1 { 0x0000040B, { 1, 0, 0x0E5AC0 }, 0x06FDE800, 0x000B4FB8 }  /* 400E5AC0 */
#define ASSET_BOWSER_MG2 { 0x0000040B, { 1, 0, 0x131200 }, 0x07093800, 0x000A317C }  /* 40131200 */
#define ASSET_WARIO_MG0  { 0x0000040B, { 1, 0, 0x11B660 }, 0x07137000, 0x000D0294 }  /* 4011B660 */
#define ASSET_WARIO_MG1  ASSET_WARIO_MG0        /* Wario shares one blob across variants */
#define ASSET_WARIO_MG2  ASSET_WARIO_MG0
#define ASSET_YOSHI_MG0  { 0x0000040B, { 1, 0, 0x141C60 }, 0x07207800, 0x000CE904 }  /* 40141C60 */
#define ASSET_YOSHI_MG1  { 0x0000040B, { 1, 0, 0x13CA40 }, 0x072D6800, 0x000CA560 }  /* 4013CA40 */
#define ASSET_YOSHI_MG2  ASSET_YOSHI_MG0
#define ASSET_PEACH_MG0  { 0x0000040B, { 1, 0, 0x16C138 }, 0x073A1000, 0x000ED41C }  /* 4016C138 */
#define ASSET_PEACH_MG1  { 0x0000040B, { 1, 0, 0x1157B8 }, 0x0748E800, 0x000CA83C }  /* 401157B8 */
#define ASSET_PEACH_MG2  ASSET_PEACH_MG0
#define ASSET_DK_MG0     { 0x0000040B, { 1, 0, 0x0EB1C0 }, 0x07559800, 0x000A3510 }  /* 400EB1C0 */
#define ASSET_DK_MG1     ASSET_DK_MG0
#define ASSET_DK_MG2     ASSET_DK_MG0
#define ASSET_TOY_MG0    { 0x0000040B, { 1, 0, 0x0D04E0 }, 0x075FD000, 0x00087390 }  /* 400D04E0 */
#define ASSET_TOY_MG1    ASSET_TOY_MG0
#define ASSET_TOY_MG2    ASSET_TOY_MG0

/* The stock table in slot order, so a swap can be undone when CGECKO_ACTIVE is a runtime flag. */
static const AssetLoadInstructions kStockAssets[7][STADIUM_VARIANTS] = {
    { ASSET_MARIO_MG0,  ASSET_MARIO_MG1,  ASSET_MARIO_MG2  },
    { ASSET_BOWSER_MG0, ASSET_BOWSER_MG1, ASSET_BOWSER_MG2 },
    { ASSET_WARIO_MG0,  ASSET_WARIO_MG1,  ASSET_WARIO_MG2  },
    { ASSET_YOSHI_MG0,  ASSET_YOSHI_MG1,  ASSET_YOSHI_MG2  },
    { ASSET_PEACH_MG0,  ASSET_PEACH_MG1,  ASSET_PEACH_MG2  },
    { ASSET_DK_MG0,     ASSET_DK_MG1,     ASSET_DK_MG2     },
    { ASSET_TOY_MG0,    ASSET_TOY_MG1,    ASSET_TOY_MG2    },
};

typedef struct {
    u8 stadium;                     /* STADIUM_ID whose slot is replaced      */
    u8 variant;                     /* 0 = normal game, 1/2 = minigame cuts   */
    AssetLoadInstructions asset;    /* the file it loads instead              */
} StadiumAssetSwap;

/* EDIT HERE: one row per (stadium, variant) slot to redirect. */
static const StadiumAssetSwap kSwaps[] = {
    { STADIUM_ID_MARIO_STADIUM, 0, ASSET_BOWSER_MG2 },
    { STADIUM_ID_MARIO_STADIUM, 1, ASSET_BOWSER_MG2 },
    { STADIUM_ID_MARIO_STADIUM, 2, ASSET_BOWSER_MG2 },
};
#define N_SWAPS ((int)(sizeof(kSwaps) / sizeof(kSwaps[0])))

CGECKO(StadiumAssetSwapCode, .state = MSSB_ALWAYS,
       .notes = "Stadiums load another stadium's field.\n"
                "Ships set to show Bowser's Castle's field in Mario Stadium.\n"
                "Props and lighting still come from the original stadium.");
void StadiumAssetSwapCode(void)
{
    bool on = CGECKO_ACTIVE;
    int i;

    for (i = 0; i < N_SWAPS; i++)
    {
        u8 stadium = kSwaps[i].stadium, variant = kSwaps[i].variant;

        StadiumFiles[stadium * STADIUM_VARIANTS + variant] =
            on ? kSwaps[i].asset : kStockAssets[stadium][variant];
    }
}
