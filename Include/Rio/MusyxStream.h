/*###########################################################
# MusyxStream.h -- stream a raw DSP-ADPCM blob off the disc through MusyX
###########################################################*/
// Author: LittleCoaks
//
// Drives MusyX's own ADPCM stream engine (whose setup API is dead-stripped
// from the DOL) by hand: an ARAM ring, a blocked voice, and a DVD async pump.
// The includer names the song BEFORE including this, then includes it from
// ONE hook only:
//
//     #define MUSYXSTREAM_FILE        "ZZZZ.dat"   // disc path
//     #define MUSYXSTREAM_DATA_OFFSET 0x...        // first ADPCM frame, in the file
//     #define MUSYXSTREAM_DATA_BYTES  0x...        // whole frames, 32-aligned
//     #define MUSYXSTREAM_FRQ         32000
//     #define MUSYXSTREAM_COEF        myCoefs      // static const s16[16]
//     #define MUSYXSTREAM_STID        0x...        // any non-zero stream id
//
// Optional: MUSYXSTREAM_RING_BYTES (8192), MUSYXSTREAM_DVD_PRIO (2).
// Then MusyxStream_Start(), MusyxStream_Pump() every frame,
// MusyxStream_Stop() to end; MusyxStream_Active() covers priming too.
// See docs/musyx_stream.md.

#ifndef MUSYXSTREAM_H
#define MUSYXSTREAM_H

#include "CGecko/Common.h"
#include "Include/types.h"
#include "Include/Dolphin/dvd.h"
#include "Include/Dolphin/OS/OSCache.h"
#include "Include/Symbols/dol.h"
#include "Include/musyx/synth.h"

#if !defined(MUSYXSTREAM_FILE) || !defined(MUSYXSTREAM_DATA_OFFSET) || \
    !defined(MUSYXSTREAM_DATA_BYTES) || !defined(MUSYXSTREAM_FRQ) || \
    !defined(MUSYXSTREAM_COEF) || !defined(MUSYXSTREAM_STID)
#error "MusyxStream.h: define MUSYXSTREAM_FILE/DATA_OFFSET/DATA_BYTES/FRQ/COEF/STID before including"
#endif
#ifndef MUSYXSTREAM_RING_BYTES
#define MUSYXSTREAM_RING_BYTES 8192u
#endif
#ifndef MUSYXSTREAM_DVD_PRIO
#define MUSYXSTREAM_DVD_PRIO 2
#endif

// ---- the ring ----------------------------------------------------------------
static u8 s_msxRing[MUSYXSTREAM_RING_BYTES] __attribute__((aligned(32)));
#define MSX_RING          ((u32)s_msxRing)
#define MSX_RING_BYTES    MUSYXSTREAM_RING_BYTES
#define MSX_RING_SAMPLES  ((MSX_RING_BYTES / 8u) * 14u)
#define MSX_READ_MIN      1024u                    /* do not bother below this   */
#define MSX_READ_MAX      4096u                    /* one DVD request            */

// ---- MusyX internals the sync does not bind ------------------------------------
/* streamInfo: STREAM_INFO[64], indexed BY VOICE (musyx/musyx_priv.h layout). */
#define MSX_STREAM_INFO(v) ((STREAM_INFO*)(0x8030F970u + (v) * sizeof(STREAM_INFO)))
_Static_assert(sizeof(STREAM_INFO) == 0x68, "STREAM_INFO stride is not 0x68");
_Static_assert(offsetof(STREAM_INFO, adpcmInfo) == 0x24, "STREAM_INFO.adpcmInfo is not at 0x24");
_Static_assert(offsetof(STREAM_INFO, voice) == 0x4C, "STREAM_INFO.voice is not at 0x4C");
_Static_assert(offsetof(STREAM_INFO, studio) == 0x60, "STREAM_INFO.studio is not at 0x60");

/* The DOL strides SYNTH_VOICE by 0x458 (mulli r7,r3,0x458), not the decomp's sizeof. */
#define MSX_SV_STRIDE     0x458u
#define MSX_SV(v)         ((SYNTH_VOICE*)((u8*)synthVoice + (v) * MSX_SV_STRIDE))
_Static_assert(offsetof(SYNTH_VOICE, addr) == 0x34, "SYNTH_VOICE.addr is not at 0x34");
_Static_assert(offsetof(SYNTH_VOICE, id) == 0xF4, "SYNTH_VOICE.id is not at 0xF4");
_Static_assert(offsetof(SYNTH_VOICE, allocId) == 0x100 && sizeof(((SYNTH_VOICE*)0)->allocId) == 4, "SYNTH_VOICE.allocId is not a u32 at 0x100");
_Static_assert(offsetof(SYNTH_VOICE, block) == 0x11C, "SYNTH_VOICE.block is not at 0x11C");
_Static_assert(offsetof(SynthInfo, voiceNum) == 0x210, "SynthInfo.voiceNum is not at 0x210");

/* ARAM stream-buffer pool: aramStreamBuffers[64], and its idle/used lists. */
typedef struct MsxAramStreamBuffer {
    /* 0x00 */ u32 next;
    /* 0x04 */ u32 aram;
    /* 0x08 */ u32 length;
    /* 0x0C */ u32 allocLength;
} MsxAramStreamBuffer;                             /* size 0x10 */
#define MSX_ARAM_BUFFERS   aramStreamBuffers_ADDR
#define MSX_ARAM_IDLE      VAR_ADDRESS(u32, aramIdleStreamBuffers_ADDR)
#define MSX_ARAM_USED      VAR_ADDRESS(u32, aramUsedStreamBuffers_ADDR)
#define MSX_ARAM_STREAM    VAR_ADDRESS(u32, aramStream_ADDR)
#define aramSyncTransferQueue FUNCTION_ADDRESS(void, aramSyncTransferQueue_ADDR, void)

// ---- state (all zero at load; this header's hook is the only writer) --------
static u8  s_msxPhase;           /* 0 idle, 1 priming, 2 running               */
static u8  s_msxVoice;           /* raw voice index = streamInfo index         */
static u8  s_msxHwbufPlus1;      /* ARAM ring handle + 1; 0 = none yet         */
static u8  s_msxReadBusy;        /* a DVD read is in flight                    */
static u32 s_msxReadLen;         /* its length                                 */
static u32 s_msxFill;            /* ring byte offset the next read lands at    */
static u32 s_msxSongPos;         /* next song byte to fetch                    */
static volatile u32 s_msxAvail;  /* fresh bytes ahead of the engine's cursor;  */
                                 /* shared with the audio IRQ, edit with IRQs off */
static DVDFileInfo s_msxFile;

static u32 MusyxStream_Active(void) { return s_msxPhase != 0; }

/* Refill callback, run by streamHandle from the audio interrupt. Zero-copy:
   reports how much of the ring is already fresh, in whole 32-byte units. */
static s32 MusyxStream_Feed(void* buffer1, u32 len1, void* buffer2, u32 len2, void* user)
{
    u32 want, have;
    (void)buffer1; (void)buffer2; (void)len2; (void)user;

    if (s_msxPhase != 2)
        return 0;
    want = (len1 / 14u) * 8u;                 /* samples -> bytes            */
    have = s_msxAvail;
    if (want > have)
        want = have;
    want &= ~31u;
    s_msxAvail = have - want;
    return (s32)((want / 8u) * 14u);          /* bytes -> samples            */
}

/* The stripped aramAllocateStreamBuffer(): fresh-carve path only. */
static s32 MusyxStream_AllocAram(u32 len)
{
    MsxAramStreamBuffer* sb;

    len = (len + 31u) & ~31u;
    sb = (MsxAramStreamBuffer*)MSX_ARAM_IDLE;
    if (sb == 0)
        return -1;
    MSX_ARAM_IDLE   = sb->next;                /* pop the idle list       */
    sb->allocLength = len;
    sb->length      = len;
    MSX_ARAM_STREAM -= len;                    /* ARAM grows downward     */
    sb->aram        = MSX_ARAM_STREAM;
    sb->next        = MSX_ARAM_USED;           /* push onto the used list */
    MSX_ARAM_USED   = (u32)sb;
    return (s32)(((u32)sb - MSX_ARAM_BUFFERS) / sizeof(MsxAramStreamBuffer));
}

/* The stripped voiceBlock(): what sndStreamActivate really uses. */
static u32 MusyxStream_VoiceBlock(u8 prio)
{
    u32 v = voiceAllocate(prio, 0xFF, 0xFFFF, 1);
    SYNTH_VOICE* sv;

    if (v == 0xFFFFFFFFu || v >= synthInfo.voiceNum)
        return 0xFFFFFFFFu;
    sv = MSX_SV(v);
    sv->block   = 1;
    sv->fxFlag  = 1;
    sv->allocId = 0xFFFF;
    vidRemoveVoiceReferences(sv);
    sv->id = v | 0xFFFFFF00u;
    if (hwIsActive(v))
        hwBreak((s32)v);
    macMakeInactive(sv, MAC_STATE_STOPPED);
    sv->addr = 0;
    voiceSetPriority(sv, prio);
    return v;
}

/* Stop and release everything but the ARAM ring. Safe to call when idle. */
static void MusyxStream_Stop(void)
{
    if (s_msxPhase == 0)
        return;
    if (s_msxReadBusy)
        DVDCancelAsync(&s_msxFile.cBlock, NULL);
    if (s_msxPhase == 2)
    {
        hwDisableIrq();
        s_msxPhase = 0;                        /* the callback returns 0 now */
        voiceUnblock(s_msxVoice);              /* what sndStreamDeactivate does */
        MSX_STREAM_INFO(s_msxVoice)->state = 0; /* ... then sndStreamFree     */
        hwEnableIrq();
    }
    s_msxPhase = 0;
    s_msxAvail = 0;
}

/* Issue the read that primes the whole ring; the pump finishes the job. */
static void MusyxStream_Start(void)
{
    if (s_msxPhase != 0)
        return;
    if (s_msxReadBusy)                         /* a cancelled read from the last Stop */
    {
        s32 st = DVDGetCommandBlockStatus(&s_msxFile.cBlock);
        if (st == DVD_STATE_BUSY || st == DVD_STATE_WAITING)
            return;                            /* try again next frame     */
        s_msxReadBusy = 0;
    }
    if (!DVDOpen((char*)MUSYXSTREAM_FILE, &s_msxFile))   /* FST lookup only; never sleeps */
        return;

    DCInvalidateRange((void*)MSX_RING, MSX_RING_BYTES);
    if (!DVDReadAsyncPrio(&s_msxFile, (void*)MSX_RING, (s32)MSX_RING_BYTES,
                          (s32)MUSYXSTREAM_DATA_OFFSET, NULL, MUSYXSTREAM_DVD_PRIO))
        return;
    s_msxReadLen  = MSX_RING_BYTES;
    s_msxReadBusy = 1;
    s_msxPhase    = 1;
}

/* Phase 1 -> 2, once the priming read has landed. */
static void MusyxStream_Prime(void)
{
    STREAM_INFO* si;
    u32 voice;
    s32 hwbuf;
    u8  pan = 64, span = 64;
    int i;

    s_msxSongPos  = MSX_RING_BYTES;
    s_msxFill     = 0;                         /* next read lands at 0, once played */
    s_msxAvail    = 0;                         /* NOT the ring size: the engine flushes the primed ring itself */

    hwDisableIrq();

    if (s_msxHwbufPlus1 == 0)
    {
        hwbuf = MusyxStream_AllocAram(MSX_RING_BYTES);
        if (hwbuf < 0)
        {
            hwEnableIrq();
            s_msxPhase = 0;
            return;
        }
        s_msxHwbufPlus1 = (u8)(hwbuf + 1);
    }

    voice = MusyxStream_VoiceBlock(0x7F);
    if (voice == 0xFFFFFFFFu)
    {
        hwEnableIrq();
        s_msxPhase = 0;
        return;
    }
    s_msxVoice = (u8)voice;

    si = MSX_STREAM_INFO(voice);
    for (i = 0; i < (int)sizeof(STREAM_INFO); i++)
        ((u8*)si)[i] = 0;

    si->nextStreamHandle = 0xFFFFFFFFu;        /* no paired (stereo) stream */
    si->stid             = MUSYXSTREAM_STID;
    si->flags            = 1;                  /* bit0 = ADPCM              */
    si->buffer           = (s16*)MSX_RING;
    si->size             = MSX_RING_SAMPLES;
    si->bytes            = MSX_RING_BYTES;
    si->last             = 0;
    si->updateFunction   = &MusyxStream_Feed;
    si->voice            = voice;
    si->frq              = MUSYXSTREAM_FRQ;
    si->user             = 0;
    si->type             = 1;                  /* 1 = ADPCM                 */
    si->hwStreamHandle   = (u8)(s_msxHwbufPlus1 - 1);
    si->prio             = 0x7F;
    si->studio           = 0;

    si->orgPan  = pan;
    si->orgSPan = span;
    if (synthFlags & 1)       { pan = 64; span = 0; }
    else if (!(synthFlags & 2)) { span = 0; }
    si->vol  = 127;
    si->pan  = pan;
    si->span = span;
    si->auxa = 0;
    si->auxb = 0;

    si->adpcmInfo.numCoef   = 8;
    si->adpcmInfo.initialPS = *(u8*)MSX_RING;
    si->adpcmInfo.loopPS    = *(u8*)MSX_RING;
    si->adpcmInfo.loopY0    = 0;
    si->adpcmInfo.loopY1    = 0;
    for (i = 0; i < 16; i++)
        si->adpcmInfo.coefTab[i / 2][i % 2] = MUSYXSTREAM_COEF[i];

    s_msxPhase = 2;
    si->state = 1;                             /* the engine takes it from here */
    hwEnableIrq();
}

/* Per frame: retire the in-flight read, then read into the ring's free region. */
static void MusyxStream_Pump(void)
{
    u32 cpos, free, len, avail;

    if (s_msxPhase == 0)
        return;

    if (s_msxReadBusy)
    {
        s32 st = DVDGetCommandBlockStatus(&s_msxFile.cBlock);
        if (st == DVD_STATE_BUSY || st == DVD_STATE_WAITING)
            return;
        s_msxReadBusy = 0;
        if (st != DVD_STATE_END)
        {
            MusyxStream_Stop();                /* drive error: stand down  */
            return;
        }
        if (s_msxPhase == 1)
        {
            MusyxStream_Prime();               /* the ring is full: go live */
            return;
        }
        hwDisableIrq();
        s_msxAvail += s_msxReadLen;
        hwEnableIrq();
        s_msxFill = (s_msxFill + s_msxReadLen) & (MSX_RING_BYTES - 1u);
        s_msxSongPos += s_msxReadLen;
        if (s_msxSongPos >= MUSYXSTREAM_DATA_BYTES)
            s_msxSongPos = 0;                  /* loop                     */
    }

    if (s_msxPhase != 2)
        return;                                /* still priming            */

    cpos  = ((hwGetPos(s_msxVoice) / 14u) * 8u) & ~31u;
    free  = (cpos - s_msxFill) & (MSX_RING_BYTES - 1u);    /* behind the play head */
    avail = s_msxAvail;
    if (free > MSX_RING_BYTES - avail)
        free = MSX_RING_BYTES - avail;                      /* and already claimed  */
    free &= ~31u;
    if (free < MSX_READ_MIN)                   /* judged BEFORE the contiguity clip, or a short tail deadlocks */
        return;

    len = free;
    if (s_msxFill + len > MSX_RING_BYTES)
        len = MSX_RING_BYTES - s_msxFill;                   /* contiguous only      */
    if (len > MSX_READ_MAX)
        len = MSX_READ_MAX;
    if (len > MUSYXSTREAM_DATA_BYTES - s_msxSongPos)
        len = MUSYXSTREAM_DATA_BYTES - s_msxSongPos;

    aramSyncTransferQueue();
    DCInvalidateRange((void*)(MSX_RING + s_msxFill), len);
    if (!DVDReadAsyncPrio(&s_msxFile, (void*)(MSX_RING + s_msxFill), (s32)len,
                          (s32)(MUSYXSTREAM_DATA_OFFSET + s_msxSongPos), NULL, MUSYXSTREAM_DVD_PRIO))
    {
        MusyxStream_Stop();
        return;
    }
    s_msxReadLen  = len;
    s_msxReadBusy = 1;
}

#endif /* MUSYXSTREAM_H */
