/*###########################################################
# LettersStream.h -- the unused ZZZZ.dat song, streamed through MusyX
###########################################################*/
// Author: LittleCoaks
//
// Plays the orphan "Letters" song out of ZZZZ.dat through MusyX's own ADPCM
// stream engine. Include from ONE hook only and call LettersStream_Start(),
// then LettersStream_Pump() every frame, LettersStream_Stop() to end;
// LettersStream_Active() covers priming too. See docs/musyx_stream.md.
#ifndef LETTERSSTREAM_H
#define LETTERSSTREAM_H

#include "CGecko/Common.h"
#include "Include/types.h"
#include "Include/Dolphin/dvd.h"

// ---- the song ----------------------------------------------------------------
#define LS_SONG_BLOB   0x08F2E808u                 /* within ZZZZ.dat            */
#define LS_SONG_DATA   (LS_SONG_BLOB + 0x88u)      /* past AdGCForm + .dsp hdr   */
#define LS_SONG_BYTES  0x507340u                   /* whole frames, 32-aligned   */
#define LS_SONG_FRQ    32000u

// ---- the ring ----------------------------------------------------------------
#define LS_RING_BYTES  8192u
static u8 s_lsRing[LS_RING_BYTES] __attribute__((aligned(32)));
#define LS_RING        ((u32)s_lsRing)
#define LS_RING_SAMPLES ((LS_RING_BYTES / 8u) * 14u)   /* 14336 */
#define LS_READ_MIN    1024u                       /* do not bother below this   */
#define LS_READ_MAX    4096u                       /* one DVD request            */
#define LS_DVD_PRIO    2

// ---- MusyX internals ---------------------------------------------------------
#define LS_STREAMINFO      0x8030F970u             /* STREAM_INFO[64], stride 0x68, indexed BY VOICE */
#define LS_SI_STRIDE       0x68u
#define LS_SI(v)           ((u8*)(LS_STREAMINFO + (v) * LS_SI_STRIDE))
#define SI_NEXT   0x00
#define SI_STID   0x04
#define SI_FLAGS  0x08
#define SI_STATE  0x0C
#define SI_TYPE   0x0D
#define SI_HWBUF  0x0E
#define SI_UPDATE 0x10
#define SI_BUFFER 0x14
#define SI_SIZE   0x18
#define SI_BYTES  0x1C
#define SI_LAST   0x20
#define SI_ADPCM  0x24      /* SNDADPCMinfo: u16 numCoef, u8 initialPS, u8 loopPS, s16 y0, s16 y1, s16 coef[16] */
#define SI_VOICE  0x4C
#define SI_USER   0x50
#define SI_FRQ    0x54
#define SI_PRIO   0x58
#define SI_VOL    0x59
#define SI_PAN    0x5A
#define SI_SPAN   0x5B
#define SI_AUXA   0x5C
#define SI_AUXB   0x5D
#define SI_ORGPAN 0x5E
#define SI_ORGSPAN 0x5F
#define SI_STUDIO 0x60

#define g_lsSynthVoice  VAR_ADDRESS(u8*, 0x803CC278)   /* POINTER to SYNTH_VOICE[], stride 0x458 */
#define LS_SV_STRIDE    0x458u
#define SV_ADDR    0x34     /* MSTEP*                        */
#define SV_ID      0xF4     /* u32                           */
#define SV_ALLOCID 0x100    /* u32                           */
#define SV_BLOCK   0x11C    /* u8                            */
#define SV_FXFLAG  0x11D    /* u8                            */

#define g_lsSynthFlags  VAR_ADDRESS(u32, 0x803CC274)
#define g_lsVoiceNum    VAR_ADDRESS(u8,  0x8030E7D8)   /* synthInfo + 0x210 */

/* ARAM stream-buffer pool: 64 x {next, aram, length, allocLength} */
#define LS_ARAM_SB      0x80326938u
#define LS_ARAM_STRIDE  0x10u
#define g_lsAramIdle    VAR_ADDRESS(u32, 0x803CC3A8)
#define g_lsAramUsed    VAR_ADDRESS(u32, 0x803CC3B0)
#define g_lsAramStream  VAR_ADDRESS(u32, 0x803CC3BC)

/* Retail entry points, LS_-prefixed to stay clear of the musyx header inlines. */
#define LS_voiceAllocate            ((u32  (*)(u8, u8, u16, u8))0x800D0B00)
#define LS_voiceUnblock             ((void (*)(u32))0x800D10EC)
#define LS_vidRemoveVoiceReferences ((void (*)(void*))0x800CFFF8)
#define LS_voiceSetPriority         ((void (*)(void*, u8))0x800D052C)
#define LS_macMakeInactive          ((void (*)(void*, u32))0x800CF97C)
#define LS_hwIsActive               ((u32  (*)(u32))0x800DC97C)
#define LS_hwBreak                  ((void (*)(u32))0x800DCB6C)
#define LS_hwGetPos                 ((u32  (*)(u32))0x800DD3DC)
#define LS_hwDisableIrq             ((void (*)(void))0x800DE778)
#define LS_hwEnableIrq              ((void (*)(void))0x800DE740)
#define LS_aramSyncTransferQueue    ((void (*)(void))0x800DDA1C)
#define LS_DCInvalidateRange        ((void (*)(void*, u32))0x8006E868)
#define LS_MAC_STATE_STOPPED        2

/* The song's own .dsp header coefficients (blob +0x44). */
static const s16 kLettersCoef[16] = {
    -184,   95, 1855, -724,  783,  609, 2500, -654,
     906, -480, 2398, -799, 1460,  366, 2801, -818
};

// ---- state (all zero at load; this header's hook is the only writer) --------
static u8  s_lsPhase;           /* 0 idle, 1 priming, 2 running               */
static u8  s_lsVoice;           /* raw voice index = streamInfo index        */
static u8  s_lsHwbufPlus1;      /* ARAM ring handle + 1; 0 = none yet         */
static u8  s_lsReadBusy;        /* a DVD read is in flight                    */
static u32 s_lsReadLen;         /* its length                                 */
static u32 s_lsFill;            /* ring byte offset the next read lands at    */
static u32 s_lsSongPos;         /* next song byte to fetch                    */
static volatile u32 s_lsAvail;  /* fresh bytes ahead of the engine's cursor;  */
                                /* shared with the audio IRQ, edit with IRQs off */
static DVDFileInfo s_lsFile;

static u32 LettersStream_Active(void) { return s_lsPhase != 0; }

/* Refill callback, run by streamHandle from the audio interrupt. Zero-copy:
   reports how much of the ring is already fresh, in whole 32-byte units. */
static s32 LettersFeed(void* buffer1, u32 len1, void* buffer2, u32 len2, void* user)
{
    u32 want, have;
    (void)buffer1; (void)buffer2; (void)len2; (void)user;

    if (s_lsPhase != 2)
        return 0;
    want = (len1 / 14u) * 8u;                 /* samples -> bytes            */
    have = s_lsAvail;
    if (want > have)
        want = have;
    want &= ~31u;
    s_lsAvail = have - want;
    return (s32)((want / 8u) * 14u);          /* bytes -> samples            */
}

/* The stripped aramAllocateStreamBuffer(): fresh-carve path only. */
static s32 LettersAllocAram(u32 len)
{
    u32 sb;

    len = (len + 31u) & ~31u;
    sb = g_lsAramIdle;
    if (sb == 0)
        return -1;
    g_lsAramIdle = *(u32*)(sb + 0x00);         /* pop the idle list       */
    *(u32*)(sb + 0x0C) = len;                  /* allocLength             */
    *(u32*)(sb + 0x08) = len;                  /* length                  */
    g_lsAramStream -= len;                     /* ARAM grows downward     */
    *(u32*)(sb + 0x04) = g_lsAramStream;       /* aram                    */
    *(u32*)(sb + 0x00) = g_lsAramUsed;         /* push onto the used list */
    g_lsAramUsed = sb;
    return (s32)((sb - LS_ARAM_SB) / LS_ARAM_STRIDE);
}

/* The stripped voiceBlock(): what sndStreamActivate really uses. */
static u32 LettersVoiceBlock(u8 prio)
{
    u32 v = LS_voiceAllocate(prio, 0xFF, 0xFFFF, 1);
    u8* sv;

    if (v == 0xFFFFFFFFu || v >= g_lsVoiceNum)
        return 0xFFFFFFFFu;
    sv = g_lsSynthVoice + v * LS_SV_STRIDE;
    sv[SV_BLOCK]  = 1;
    sv[SV_FXFLAG] = 1;
    *(u32*)(sv + SV_ALLOCID) = 0xFFFF;
    LS_vidRemoveVoiceReferences(sv);
    *(u32*)(sv + SV_ID) = v | 0xFFFFFF00u;
    if (LS_hwIsActive(v))
        LS_hwBreak(v);
    LS_macMakeInactive(sv, LS_MAC_STATE_STOPPED);
    *(u32*)(sv + SV_ADDR) = 0;
    LS_voiceSetPriority(sv, prio);
    return v;
}

/* Stop and release everything but the ARAM ring. Safe to call when idle. */
static void LettersStream_Stop(void)
{
    if (s_lsPhase == 0)
        return;
    if (s_lsReadBusy)
        DVDCancelAsync(&s_lsFile.cBlock, NULL);
    if (s_lsPhase == 2)
    {
        LS_hwDisableIrq();
        s_lsPhase = 0;                         /* the callback returns 0 now */
        LS_voiceUnblock(s_lsVoice);            /* what sndStreamDeactivate does */
        LS_SI(s_lsVoice)[SI_STATE] = 0;        /* ... then sndStreamFree      */
        LS_hwEnableIrq();
    }
    s_lsPhase = 0;
    s_lsAvail = 0;
}

/* Issue the read that primes the whole ring; the pump finishes the job. */
static void LettersStream_Start(void)
{
    if (s_lsPhase != 0)
        return;
    if (s_lsReadBusy)                          /* a cancelled read from the last Stop */
    {
        s32 st = DVDGetCommandBlockStatus(&s_lsFile.cBlock);
        if (st == DVD_STATE_BUSY || st == DVD_STATE_WAITING)
            return;                            /* try again next frame     */
        s_lsReadBusy = 0;
    }
    if (!DVDOpen("ZZZZ.dat", &s_lsFile))       /* FST lookup only; never sleeps */
        return;

    LS_DCInvalidateRange((void*)LS_RING, LS_RING_BYTES);
    if (!DVDReadAsyncPrio(&s_lsFile, (void*)LS_RING, (s32)LS_RING_BYTES,
                          (s32)LS_SONG_DATA, NULL, LS_DVD_PRIO))
        return;
    s_lsReadLen  = LS_RING_BYTES;
    s_lsReadBusy = 1;
    s_lsPhase    = 1;
}

/* Phase 1 -> 2, once the priming read has landed. */
static void LettersStream_Prime(void)
{
    u8* si;
    u32 voice;
    s32 hwbuf;
    u8  pan = 64, span = 64;
    int i;

    s_lsSongPos  = LS_RING_BYTES;
    s_lsFill     = 0;                          /* next read lands at 0, once played */
    s_lsAvail    = 0;                          /* NOT LS_RING_BYTES: the engine flushes the primed ring itself */

    LS_hwDisableIrq();

    if (s_lsHwbufPlus1 == 0)
    {
        hwbuf = LettersAllocAram(LS_RING_BYTES);
        if (hwbuf < 0)
        {
            LS_hwEnableIrq();
            s_lsPhase = 0;
            return;
        }
        s_lsHwbufPlus1 = (u8)(hwbuf + 1);
    }

    voice = LettersVoiceBlock(0x7F);
    if (voice == 0xFFFFFFFFu)
    {
        LS_hwEnableIrq();
        s_lsPhase = 0;
        return;
    }
    s_lsVoice = (u8)voice;

    si = LS_SI(voice);
    for (i = 0; i < (int)LS_SI_STRIDE; i++)
        si[i] = 0;

    *(u32*)(si + SI_NEXT)   = 0xFFFFFFFFu;     /* no paired (stereo) stream */
    *(u32*)(si + SI_STID)   = 0x4C455454u;     /* 'LETT': any non-zero id   */
    *(u32*)(si + SI_FLAGS)  = 1;               /* bit0 = ADPCM              */
    *(u32*)(si + SI_BUFFER) = LS_RING;
    *(u32*)(si + SI_SIZE)   = LS_RING_SAMPLES;
    *(u32*)(si + SI_BYTES)  = LS_RING_BYTES;
    *(u32*)(si + SI_LAST)   = 0;
    *(u32*)(si + SI_UPDATE) = (u32)&LettersFeed;
    *(u32*)(si + SI_VOICE)  = voice;
    *(u32*)(si + SI_FRQ)    = LS_SONG_FRQ;
    *(u32*)(si + SI_USER)   = 0;
    si[SI_TYPE]   = 1;                         /* 1 = ADPCM                 */
    si[SI_HWBUF]  = (u8)(s_lsHwbufPlus1 - 1);
    si[SI_PRIO]   = 0x7F;
    si[SI_STUDIO] = 0;

    si[SI_ORGPAN]  = pan;
    si[SI_ORGSPAN] = span;
    if (g_lsSynthFlags & 1)       { pan = 64; span = 0; }
    else if (!(g_lsSynthFlags & 2)) { span = 0; }
    si[SI_VOL]  = 127;
    si[SI_PAN]  = pan;
    si[SI_SPAN] = span;
    si[SI_AUXA] = 0;
    si[SI_AUXB] = 0;

    *(u16*)(si + SI_ADPCM + 0x00) = 8;                    /* numCoef   */
    si[SI_ADPCM + 0x02] = *(u8*)LS_RING;                  /* initialPS */
    si[SI_ADPCM + 0x03] = *(u8*)LS_RING;                  /* loopPS    */
    *(s16*)(si + SI_ADPCM + 0x04) = 0;
    *(s16*)(si + SI_ADPCM + 0x06) = 0;
    for (i = 0; i < 16; i++)
        *(s16*)(si + SI_ADPCM + 0x08 + i * 2) = kLettersCoef[i];

    s_lsPhase = 2;
    si[SI_STATE] = 1;                          /* the engine takes it from here */
    LS_hwEnableIrq();
}

/* Per frame: retire the in-flight read, then read into the ring's free region. */
static void LettersStream_Pump(void)
{
    u32 cpos, free, len, avail;

    if (s_lsPhase == 0)
        return;

    if (s_lsReadBusy)
    {
        s32 st = DVDGetCommandBlockStatus(&s_lsFile.cBlock);
        if (st == DVD_STATE_BUSY || st == DVD_STATE_WAITING)
            return;
        s_lsReadBusy = 0;
        if (st != DVD_STATE_END)
        {
            LettersStream_Stop();              /* drive error: stand down  */
            return;
        }
        if (s_lsPhase == 1)
        {
            LettersStream_Prime();             /* the ring is full: go live */
            return;
        }
        LS_hwDisableIrq();
        s_lsAvail += s_lsReadLen;
        LS_hwEnableIrq();
        s_lsFill = (s_lsFill + s_lsReadLen) & (LS_RING_BYTES - 1u);
        s_lsSongPos += s_lsReadLen;
        if (s_lsSongPos >= LS_SONG_BYTES)
            s_lsSongPos = 0;                   /* loop                     */
    }

    if (s_lsPhase != 2)
        return;                                /* still priming            */

    cpos  = ((LS_hwGetPos(s_lsVoice) / 14u) * 8u) & ~31u;
    free  = (cpos - s_lsFill) & (LS_RING_BYTES - 1u);      /* behind the play head */
    avail = s_lsAvail;
    if (free > LS_RING_BYTES - avail)
        free = LS_RING_BYTES - avail;                       /* and already claimed  */
    free &= ~31u;
    if (free < LS_READ_MIN)                    /* judged BEFORE the contiguity clip, or a short tail deadlocks */
        return;

    len = free;
    if (s_lsFill + len > LS_RING_BYTES)
        len = LS_RING_BYTES - s_lsFill;                     /* contiguous only      */
    if (len > LS_READ_MAX)
        len = LS_READ_MAX;
    if (len > LS_SONG_BYTES - s_lsSongPos)
        len = LS_SONG_BYTES - s_lsSongPos;

    LS_aramSyncTransferQueue();
    LS_DCInvalidateRange((void*)(LS_RING + s_lsFill), len);
    if (!DVDReadAsyncPrio(&s_lsFile, (void*)(LS_RING + s_lsFill), (s32)len,
                          (s32)(LS_SONG_DATA + s_lsSongPos), NULL, LS_DVD_PRIO))
    {
        LettersStream_Stop();
        return;
    }
    s_lsReadLen  = len;
    s_lsReadBusy = 1;
}

#endif /* LETTERSSTREAM_H */
