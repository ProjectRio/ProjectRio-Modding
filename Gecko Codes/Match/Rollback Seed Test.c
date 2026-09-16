/*###########################################################
# Rollback Seed Test  (mid-play completeness probe)
###########################################################*/
// Author: LittleCoaks (harness drafted with Claude)
// Diagnostic, not a shipping feature. Method, result addresses and how to
// read them: docs/rollback.md

#include "Include/game/UnknownHomes_Game.h"

#include "Include/static/UnknownHomes_Static.h"
#include "Include/text/text_channel.h"
#define SCRATCH   0x815B4000   // verified-free high MEM1 (2 MB zero run); NOT a game buffer
#define N_SEED    21
#define HASH_LO   0x8088A7E4
#define HASH_HI   0x80893AA0

#define RT_HEART  VAR_ADDRESS(u32, 0x802EBFC0)
#define RT_MODE   VAR_ADDRESS(u32, 0x802EBFC4)
#define RT_RESULT VAR_ADDRESS(u32, 0x802EBFC8)
#define RT_HASHA  VAR_ADDRESS(u32, 0x802EBFCC)
#define RT_HASHB  VAR_ADDRESS(u32, 0x802EBFD0)

// The logical seed: 19 replay-snapshot structs + RNG + play counters.
static struct { u32 addr; u32 size; } SEED[N_SEED] = {
    {0x8089298C, 0x158}, {0x80892968, 0x024}, {0x808928A0, 0x0C8}, {0x80890B38, 0x1BF8},
    {0x808909C0, 0x178}, {0x80890910, 0x0B0}, {0x80892AE4, 0x0BC}, {0x80892750, 0x150},
    {0x80892730, 0x020}, {0x8088F368, 0x15A8},{0x8088EE18, 0x550}, {0x803537E4, 0x2AC},
    {0x803535C8, 0x21C}, {0x80892BAC, 0x014}, {0x80892F8C, 0x2F4}, {0x80893280, 0x080},
    {0x80893300, 0x00E}, {0x80893310, 0x004}, {0x80893314, 0x006},
    {0x80892684, 0x020},   // RNG state (snapshot omits it)
    {0x8088A808, 0x008},   // playFrameCounter + lastPlayFrameCounter
};

static void seed_copy(int to_scratch)
{
    u32 off = SCRATCH;
    for (int i = 0; i < N_SEED; i++) {
        u32 live = SEED[i].addr, n = SEED[i].size;
        for (u32 j = 0; j < n; j++) {
            if (to_scratch) *((u8*)(off + j)) = *((u8*)(live + j));
            else            *((u8*)(live + j)) = *((u8*)(off + j));
        }
        off += n;
    }
}

static u32 band_checksum(void)
{
    u32 h = 0;
    for (u32 a = HASH_LO; a < HASH_HI; a += 4)
        h = (h * 33) + *((u32*)a);
    return h;
}

CGECKO(RollbackSeedTest, .address = 0x80699D34, .state = MSSB_GAME,
       .notes = "Developer test code. Leave this off.\n"
                "Rollback netplay experiment.");
void RollbackSeedTest()
{
    RT_HEART = RT_HEART + 1;   // proves the hook is running at all

    if (g_GameLogic.sceneID != SCENE_ID_LIVE_BALL) { RT_MODE = 0; return; }

    if (RT_MODE == 0) {
        if ((g_InputBuffer.pads[0].button & INPUT_TRIGGER_Z) == INPUT_TRIGGER_Z)
            RT_MODE = 1;
    } else if (RT_MODE == 1) {
        seed_copy(1);
        RT_MODE = 2;
    } else if (RT_MODE == 2) {
        RT_HASHA = band_checksum();
        seed_copy(0);
        RT_MODE = 3;
    } else if (RT_MODE == 3) {
        RT_HASHB = band_checksum();
        RT_RESULT = (RT_HASHB == RT_HASHA) ? 1 : 2;
        RT_MODE = 0;
    }
}
