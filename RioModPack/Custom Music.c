/*###########################################################
# Custom Music
###########################################################*/
// Author: LittleCoaks
// See docs/custom_music.md.
#include "Include/game/UnknownHomes_Game.h"

#include "Include/static/UnknownHomes_Static.h"
#include "Include/musyx/musyx.h"
#include "Include/Rio/MenuMusic.h"
#include "RioModPack/MusicConfig.h"
#include "RioModPack/LettersStream.h"

// ---- claimed RAM (see ClaimedFreeMemory.h) --------------------------------
#define MMT_MAGIC               0x4D4D5401
#define g_mmtStarted   VAR_ADDRESS(u32, 0x802EC2B0)  // == MMT_MAGIC while streaming
#define g_mmtTrack     VAR_ADDRESS(u32, 0x802EC2BC)  // the track currently streaming
#define MMT_PATH_BUF            0x802EC2C0           // 32 bytes for the menu's path

#define g_musicMagic   VAR_ADDRESS(u32, MUSICCFG_MAGIC_ADDR)

// ---- game ------------------------------------------------------------------
#define HOST_ID         14                           // home_in: the descriptor the menu borrows
#define MUSIC_HOST_SLOT 15                           // the slot that drives it
#define JUKEBOX_WORK 0x8034E478                      // jukeboxWork (unbound): 80 bytes per stream id
#define JUKEBOX_HEAD VAR_ADDRESS(u8*, jukeboxQueueHead_ADDR)
#define MMT_WORK_BUF (JUKEBOX_WORK + HOST_ID * 80)

#define STREAM_ENTRY(id) (&VAR_ADDRESS(StreamDescriptor, MUSIC_STREAM_TABLE + (id) * sizeof(StreamDescriptor)))
#define SAVED_ENTRY(slot) ((u32*)(MUSIC_SAVED_BASE + ((slot) - 1) * 8))

/* Capture each stream's stock descriptor exactly once, before any retarget. */
static void musicInitOnce(void)
{
    int slot;

    if (g_musicMagic == MUSICCFG_MAGIC)
        return;
    g_musicMagic = MUSICCFG_MAGIC;

    for (slot = 1; slot < MUSIC_SLOT_COUNT; slot++)
    {
        StreamDescriptor* e = STREAM_ENTRY(s_musicSlotStream[slot]);
        u32* s = SAVED_ENTRY(slot);

        s[0] = (u32)e->path;
        s[1] = e->size;
    }
    MusicConfig_Reset();
}

/* What stream id `id` ORIGINALLY named: a retargetable id's live entry is not reliable. */
static void musicStockDesc(u32 id, u32* path, u32* size)
{
    int slot;

    for (slot = 1; slot < MUSIC_SLOT_COUNT; slot++)
    {
        if (s_musicSlotStream[slot] == id)
        {
            u32* s = SAVED_ENTRY(slot);
            *path = s[0];
            *size = s[1];
            return;
        }
    }
    {
        StreamDescriptor* e = STREAM_ENTRY(id);
        *path = (u32)e->path;
        *size = e->size;
    }
}

/* Resolve a track to {path, size}; 0 when it is not playable on this disc.
   `scratch` must be claimed RAM: the descriptor keeps the pointer. */
static u32 musicResolve(u32 track, char* scratch, u32* path, u32* size)
{
    if (MUSIC_IS_STOCK(track))
    {
        musicStockDesc(MUSIC_TRACK_STREAM(track), path, size);
        return 1;
    }
    if (track == MUSIC_OFF)
    {
        MusicBuildPath(track, scratch);              // a path that is NOT on the disc, on purpose
        *path = (u32)scratch;
        *size = 0;
        return 1;
    }
    if (MUSIC_IS_STAR(track) || MUSIC_IS_CUSTOM(track))
    {
        s32 len;

        MusicBuildPath(track, scratch);
        len = Fst_ProbePath(scratch);
        if (len <= 0)
            return 0;                                // not on this disc
        *path = (u32)scratch;
        *size = (u32)len;
        return 1;
    }
    return 0;                                        // Default, or out of range
}

static void musicApplySlot(int slot)
{
    StreamDescriptor* e = STREAM_ENTRY(s_musicSlotStream[slot]);
    u32* saved = SAVED_ENTRY(slot);
    char* buf  = (char*)(MUSIC_PATHBUF_BASE + (slot - 1) * MUSIC_PATHBUF_SIZE);
    u32  track = MusicSlotTrack(slot);
    u32  path  = saved[0];                           // Default, and the fallback
    u32  size  = saved[1];

    if (track != MUSIC_DEFAULT)
        musicResolve(track, buf, &path, &size);

    if ((u32)e->path != path || e->size != size)
    {
        e->path  = (const char*)path;
        e->size  = size;
        e->sizeC = size;
    }
}

/* Idempotent. The host slot is skipped while the menu stream has it borrowed. */
static void musicApplyStreams(void)
{
    int slot;

    for (slot = 1; slot < MUSIC_SLOT_COUNT; slot++)
    {
        if (slot == MUSIC_HOST_SLOT && g_mmtStarted == MMT_MAGIC)
            continue;
        musicApplySlot(slot);
    }
}

/* The stream id the game has queued behind ours, or -1. */
static s32 mmtQueuedGameStream(void)
{
    u8* work = (u8*)MMT_WORK_BUF;                    // ours -- the one to ignore
    u8* node = JUKEBOX_HEAD;
    u32 hops;

    for (hops = 0; hops < 16; hops++)
    {
        u32 desc;

        if (node < (u8*)0x80000000 || node >= (u8*)0x81800000)
            break;

        desc = *(u32*)(node + 0x4C);
        if (node != work && desc >= MUSIC_STREAM_TABLE &&
            desc < MUSIC_STREAM_TABLE + MUSIC_STREAM_COUNT * 16)
            return (s32)((desc - MUSIC_STREAM_TABLE) / 16);

        node = *(u8**)(node + 4);                    // ->next
    }
    return -1;
}

/* Stop the menu stream and hand the borrowed descriptor back. */
static void mmtStop(u32 releaseGuard)
{
    s32 queued = (releaseGuard != 0) ? mmtQueuedGameStream() : -1;

    jukeboxCmd(4);                                   // cancel the DTK stream FIRST, or it resumes mid-match
    jukeboxStop();

    g_mmtStarted = 0;                                // cleared HERE, before the
    musicApplySlot(MUSIC_HOST_SLOT);                 // apply, so its skip lifts

    if (releaseGuard != 0)
        MenuMusic_Release();                         // let the stock music start again

    if (queued >= 0)
        playStream((u8)queued);
}

/* Start the menu stream on `track`. On a probe miss nothing is touched. */
static void mmtStart(u32 track)
{
    StreamDescriptor* e = STREAM_ENTRY(HOST_ID);
    u32  path, size;

    if (!musicResolve(track, (char*)MMT_PATH_BUF, &path, &size))
        return;

    e->path  = (const char*)path;
    e->size  = size;
    e->sizeC = size;

    MenuMusic_StopVoice();

    jukeboxStop();                                   // a stale stadium track may still be head

    playStream(HOST_ID);
    g_mmtStarted = MMT_MAGIC;
    g_mmtTrack   = track;
}

CGECKO(CustomMusic,
       .notes = "Choose the music for the menu and for each stadium. Custom "
                "songs added to the disc can be picked too.");
void CustomMusic()
{
    u16 sc = inningSetting.currentScene;
    u32 want;

    musicInitOnce();

    if (inningSetting.rel != 4)
    {
        if (g_mmtStarted == MMT_MAGIC)
            mmtStop(1);
        if (LettersStream_Active())
        {
            LettersStream_Stop();
            MenuMusic_Release();                     /* hand the routine back */
        }
        return;
    }

    musicApplyStreams();

    want = MusicSlotTrack(MUSIC_SLOT_MENU);

    if (want != MUSIC_LETTERS && LettersStream_Active())
    {
        LettersStream_Stop();
        if (want == MUSIC_DEFAULT || want == MUSIC_DICTIONARY)
            MenuMusic_Release();
    }

    if (want == MUSIC_DICTIONARY)
    {
        if (g_mmtStarted == MMT_MAGIC)
            mmtStop(1);
        return;
    }

    if (sc == 7)                                     // Dictionary scene
    {
        if (g_mmtStarted == MMT_MAGIC)
            mmtStop(0);                              // keep fx 484 suppressed
        if (LettersStream_Active())
            LettersStream_Stop();                    // that scene has its own track
        menuMusic.playing = 1;
        return;
    }

    if (want == MUSIC_OFF)
    {
        if (g_mmtStarted == MMT_MAGIC)
            mmtStop(0);
        MenuMusic_Hold();
        return;
    }

    if (want == MUSIC_LETTERS)
    {
        if (g_mmtStarted == MMT_MAGIC)
            mmtStop(0);                              // the .adp host stream goes, guard stays

        if (!LettersStream_Active())
        {
            MenuMusic_StopVoice();
            LettersStream_Start();
        }
        if (LettersStream_Active())
        {
            menuMusic.playing = 1;
            LettersStream_Pump();                    // keep the ring fed
        }
        return;
    }

    if (want == MUSIC_DEFAULT)
    {
        if (g_mmtStarted == MMT_MAGIC)
            mmtStop(1);
        return;
    }

    if (g_mmtStarted == MMT_MAGIC && g_mmtTrack != want)
    {
        mmtStop(0);                                  // the new track starts next frame
        return;
    }

    if (g_mmtStarted != MMT_MAGIC)
        mmtStart(want);

    if (g_mmtStarted == MMT_MAGIC)
        menuMusic.playing = 1;
}
