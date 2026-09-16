/*###########################################################
# Stadium Asset Swap
###########################################################*/
// Author: LittleCoaks
// *Makes a stadium load another stadium's asset file from ZZZZ.dat.
// *Ships set to render Bowser's Castle's field in Mario Stadium.
#include "Include/mssbTypes.h"                    // STADIUM_ID
#include "Include/Symbols/dol.h"                  // StadiumFiles_ADDR
#include "Include/Unknown/File_0x800a64e0.h"      // AssetLoadInstructions

/* HOW THE GAME PICKS A STADIUM FILE.

   Every stadium's geometry lives in ZZZZ.dat as one LZSS blob, and the DOL
   keeps a table of where each blob is: StadiumFiles (.data 0x800EFBE8), 7
   stadiums x 3 variants of AssetLoadInstructions, 0x10 bytes each.

   manageStadiumLoading (game.rel 0x8063F588) does the lookup in its state 0:

       0x8063F5E4  addi r0, r3, -0x418     ; r0 = 0x800EFBE8 (StadiumFiles)
       0x8063F5E0  lbz  r6, 9(r5)          ; GameInitVariables.StadiumID
       0x8063F5E8  lbz  r7, 0xA(r5)        ; .miniGameStadiumIndicator
       0x8063F5F0  mulli r3, r6, 3         ; StadiumID * 3
       0x8063F5FC  slwi  r3, r3, 4         ;   ... * 0x10
       0x8063F600  add   r3, r0, r3
       0x8063F604  bl    0x800A70DC        ; loader gets the entry, verbatim

   So the entry is the whole decision -- nothing downstream re-derives the disk
   offset. Overwrite the entry and the stadium loads whatever you point it at.

   The variant index is miniGameStadiumIndicator: 0 for a normal game, 1 and 2
   for the minigame cuts of the same stadium (Bob-omb Derby and friends). Four
   of the seven stadiums use one blob for all three.

   WHAT ACTUALLY CHANGES. The blob carries the field: StadiumFileHeader, the
   models, and the collision mesh, all of which follow the swap. What does NOT
   follow is anything the game derives from StadiumID instead of from the file
   -- loadStadiumObjects (0x806F8C48) still spawns the ORIGINAL stadium's props
   and hazards, and initStadiumLighting (0x8001CBD4) still picks its lighting.
   Expect the destination's field with the source stadium's furniture, and
   expect some combinations to fault outright: the props are positioned for a
   field that is no longer there.

   THERE IS A SECOND COPY of this table in game.rel at 0x807B1DEC, read only by
   FUN_80645570. Nothing in game.rel's text branches to that function (checked
   every b/bl in the section), so the DOL table is the live one and this code
   leaves the duplicate alone.

   Applied per frame rather than once: StadiumFiles is DOL .data, so it is back
   to stock on every boot, and the write has to be in place before the match
   starts. Four words a frame is free, and the loader only ever reads the entry
   at stadium-load time. */

#define StadiumFiles ((AssetLoadInstructions *)StadiumFiles_ADDR)
#define STADIUM_VARIANTS 3

/* ---- The stock table, dumped from main.dol 0x800EFBE8 -----------------------
   The bitfield is { compressionFlag, unused, originalDiskSize }; the raw word
   is in the comment so these can be checked against a hex dump of the DOL. */
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

/* The stock table again, in slot order, so a swap can be undone. CGECKO_ACTIVE
   folds to 1 in a plain build and this whole path disappears; a pack that
   redefines it to a runtime flag (see CGecko/Common.h) gets a mod that can be
   switched off mid-session instead of only at boot. */
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

/* ---- EDIT HERE -------------------------------------------------------------
   One row per (stadium, variant) slot to redirect. Point any stadium at any
   ASSET_* above, or write a raw entry inline for a blob that is not in the
   stock table. Leave a stadium out entirely and it loads as normal. */
static const StadiumAssetSwap kSwaps[] = {
    { STADIUM_ID_MARIO_STADIUM, 0, ASSET_BOWSER_MG2 },
    { STADIUM_ID_MARIO_STADIUM, 1, ASSET_BOWSER_MG2 },
    { STADIUM_ID_MARIO_STADIUM, 2, ASSET_BOWSER_MG2 },
};
#define N_SWAPS ((int)(sizeof(kSwaps) / sizeof(kSwaps[0])))

CGECKO(StadiumAssetSwapCode, .state = MSSB_ALWAYS,
       .notes = "Stadiums load another stadium's field\n"
                "out of ZZZZ.dat. Props and lighting still\n"
                "come from the original stadium.");
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
