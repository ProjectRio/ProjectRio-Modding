/*###########################################################
# LettersStream.h -- the unused ZZZZ.dat song, streamed through MusyX
###########################################################*/
// Author: LittleCoaks
//
// Plays the orphan "Letters" song straight out of ZZZZ.dat on a stock disc, by
// reviving MusyX's own ADPCM stream engine. Included by Custom Music.c and
// driven ONLY from its CustomMusic() hook: cgecko links every hook separately,
// so a static touched from two hooks would exist twice.
//
// THE SONG. ZZZZ.dat +0x08f2e808 holds an AdGCForm blob nothing references:
// 8-byte magic + 0x20 wrapper, a standard 0x60-byte DSP-ADPCM header, then
// data at +0x88. Mono, 32000 Hz, 9,226,688 samples (4:48, fades out). It is
// NOT the raw DTK ADP the stadium themes stream as (a DTK block repeats bytes
// 0/1 at 2/3; this never does), so no stream descriptor can play it -- but it
// is exactly the sample format MusyX plays, which is what makes this possible.
//
// THE ENGINE. streamHandle (0x800C8874) is whole in the retail DOL and runs
// every few audio frames from hwHandle; only the public setup calls
// (sndStreamAllocEx / sndStreamActivate / voiceBlock / aramAllocateStreamBuffer)
// were dead-stripped, so nothing ever hands it a stream and it idles. This
// file is that setup, transcribed from the decomp: an ARAM ring, a BLOCKED
// voice, streamInfo[voice] filled in, and the game's own code does the rest.
// Every struct offset below was checked against the retail disassembly.
//
// THE REFILL CONTRACT (read out of streamHandle's state-2 path):
//   cpos = hwGetPos(voice) rounded down to a whole ADPCM frame;
//   if (last != cpos) ret = updateFunction(buf1, len1, buf2, len2, user);
//   last = (last + ret) % size; hwFlushStream(the claimed bytes) -> ARAM.
// Lengths are SAMPLES, the pointers arrive pre-offset, and a short or zero
// return is handled cleanly. After each refill the engine reads the ring's
// byte 0 (uncached) as the loop predictor, so byte 0 must always be a frame
// header and must never be rewritten ahead of the play head -- see the pump.
//
// ZERO-COPY. There is one 8 KB ring, shared: the DVD reads land directly in
// the region the DSP has already played and the engine has already flushed,
// and the callback just reports how much of that has arrived. No staging, no
// memcpy, and only the ring in RAM. Claims are made in 56-sample (32-byte)
// units so the engine's byte cursor stays 32-byte aligned for the DVD.
//
// NOTHING HERE MAY SLEEP. Gecko hooks run with OSCurrentThread == NULL, so any
// SDK call that parks the caller -- the synchronous DVDReadPrio, DVDCancel --
// faults writing thread->state (an "Invalid write to 0x000002c8" in
// OSSleepThread). Every disc operation below is the Async form, polled by the
// pump, and priming the ring is a phase of its own rather than a blocking read.
//
// THE RING is a static array in this image, 32-byte aligned as hwFlushStream
// and DVDRead require. In the DOL-baked pack that is free: the image already
// lives in a reserved region (0x817C0000, ~253 KB, a few percent used). In a
// gecko build it costs 8 KB of zeros in the code list, which is acceptable
// for a build that is never shipped that way.
//
// It was briefly lbl_802ED140, a "discrete 8192-byte .bss object referenced
// by nothing" -- which turned out to be the RENDER THREAD'S STACK (OSThread
// 0x803C7488, priority 12: stackEnd 0x802ED140, stackBase 0x802EF140). The
// priming read overwrote it and the thread returned into garbage. Every
// reference scan missed it because a stack's only reference is to its END,
// 0x802EF140, which is the address of the NEXT object. Do not go looking for
// "free" game RAM for a DMA target by reference-scanning again: dump it live
// on an unmodded boot (the repo's actual standard) or use the image.
#ifndef LETTERSSTREAM_H
#define LETTERSSTREAM_H

#include "CGecko/Common.h"
#include "Include/types.h"
#include "Include/Dolphin/dvd.h"

// ---- the song ----------------------------------------------------------------
#define LS_SONG_BLOB   0x08F2E808u                 /* within ZZZZ.dat            */
#define LS_SONG_DATA   (LS_SONG_BLOB + 0x88u)      /* past AdGCForm + .dsp hdr   */
#define LS_SONG_BYTES  0x507340u                   /* whole frames, 32-aligned:  */
                                                   /* 5,272,392 floored to 32    */
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
#define SV_ALLOCID 0x100    /* u32 -- see LettersVoiceBlock  */
#define SV_BLOCK   0x11C    /* u8                            */
#define SV_FXFLAG  0x11D    /* u8                            */

#define g_lsSynthFlags  VAR_ADDRESS(u32, 0x803CC274)
#define g_lsVoiceNum    VAR_ADDRESS(u8,  0x8030E7D8)   /* synthInfo + 0x210 */

/* ARAM stream-buffer pool: 64 x {next, aram, length, allocLength}, offsets
   read back out of aramGetStreamBufferAddress. */
#define LS_ARAM_SB      0x80326938u
#define LS_ARAM_STRIDE  0x10u
#define g_lsAramIdle    VAR_ADDRESS(u32, 0x803CC3A8)
#define g_lsAramUsed    VAR_ADDRESS(u32, 0x803CC3B0)
#define g_lsAramStream  VAR_ADDRESS(u32, 0x803CC3BC)

/* Retail entry points. Raw pointers with an LS_ prefix so nothing here can
   collide with the musyx header inlines Custom Music.c already pulls in. */
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

/* The song's own .dsp header supplies SNDADPCMinfo verbatim: its 16
   coefficients (blob +0x44). The initial predictor/scale byte is data[0]. */
static const s16 kLettersCoef[16] = {
    -184,   95, 1855, -724,  783,  609, 2500, -654,
     906, -480, 2398, -799, 1460,  366, 2801, -818
};

// ---- state (all zero at load; this header's hook is the only writer) --------
static u8  s_lsPhase;           /* 0 idle, 1 priming (first read in flight),  */
                                /* 2 running (engine owns the voice)          */
static u8  s_lsVoice;           /* raw voice index = streamInfo index        */
static u8  s_lsHwbufPlus1;      /* ARAM ring handle + 1; 0 = none yet. Kept   */
                                /* for the session: the free path was         */
                                /* stripped and 8 KB of ARAM is nothing.      */
static u8  s_lsReadBusy;        /* a DVD read is in flight                    */
static u32 s_lsReadLen;         /* its length                                 */
static u32 s_lsFill;            /* ring byte offset the next read lands at    */
static u32 s_lsSongPos;         /* next song byte to fetch                    */
static volatile u32 s_lsAvail;  /* fresh bytes ahead of the engine's cursor,  */
                                /* not yet claimed. Written by the callback   */
                                /* (audio IRQ) and the pump (main), so the    */
                                /* pump edits it with IRQs off.               */
static DVDFileInfo s_lsFile;

/* "Active" covers priming too: the caller keeps the guard held and keeps
   pumping from the first read onward, and does not try to start again. */
static u32 LettersStream_Active(void) { return s_lsPhase != 0; }

/* The refill callback, called by streamHandle from the audio interrupt with
   the region [last, cpos) to fill. Zero-copy: the data is already there (the
   pump put it there), so this only reports how much, in whole 32-byte units,
   never more than the contiguous len1 -- so the engine always takes its
   single-flush path and byte cursors stay aligned. Its address is stored in
   streamInfo, which needs CGecko >= 70ae670 (before that, &function pointed
   inside the payload). */
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

/* Transcription of the stripped aramAllocateStreamBuffer(): the "carve a
   fresh one off the top" path only -- nothing else allocates these, so the
   free list is always empty. */
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

/* Transcription of the stripped voiceBlock() (synthvoice.c) -- what
   sndStreamActivate really uses, NOT a bare voiceAllocate. block=1 is what
   keeps voice stealing from handing this voice to the next SFX; the sentinel
   id / allocId keep the by-id lookups from ever matching it. voiceAllocate
   returns the RAW index (or 0xFFFFFFFF); the composite id is written here.
   allocId is a u32: mcmdPlayMacro loads it with lwz and passes it straight
   through, which a u16 would never compile to. */
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

/* Stop and release everything but the ARAM ring. Safe to call when idle. A
   read still in flight is cancelled ASYNCHRONOUSLY and left marked busy: the
   next Start waits for the command block to go idle before reusing it. The
   file is never closed -- DVDClose is bookkeeping-free on this SDK and a
   later DVDOpen simply refills the same DVDFileInfo. */
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

/* Start from the top of the song: issue the read that primes the whole ring
   and go to phase 1. The pump finishes the job when that read lands. */
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

    /* Invalidate first -- hwFlushStream will DCStoreRange this region before
       its ARAM DMA, and a stale line would write old bytes back over the
       fresh read. */
    LS_DCInvalidateRange((void*)LS_RING, LS_RING_BYTES);
    if (!DVDReadAsyncPrio(&s_lsFile, (void*)LS_RING, (s32)LS_RING_BYTES,
                          (s32)LS_SONG_DATA, NULL, LS_DVD_PRIO))
        return;
    s_lsReadLen  = LS_RING_BYTES;
    s_lsReadBusy = 1;
    s_lsPhase    = 1;
}

/* Phase 1 -> 2, once the priming read has landed: everything that has to be
   in place before the engine's first pass. The engine itself builds the
   SAMPLE_INFO, starts the voice and flushes the whole ring to ARAM on its
   next service. */
static void LettersStream_Prime(void)
{
    u8* si;
    u32 voice;
    s32 hwbuf;
    u8  pan = 64, span = 64;
    int i;

    s_lsSongPos  = LS_RING_BYTES;
    s_lsFill     = 0;                          /* next read lands at 0, once played */
    /* NOT LS_RING_BYTES: the engine flushes the primed ring to ARAM itself
       (state 1), so nothing in it is "fresh, unclaimed" data. Calling it so
       made the engine re-flush lap 1 as lap 2 -- the first half second played
       twice -- and threw the pump's accounting off by a lap. Fresh data only
       exists once the pump has written it behind the play head. */
    s_lsAvail    = 0;

    /* From here the writes race the audio interrupt, where streamHandle runs;
       sndStreamAllocEx brackets exactly this with hwDisableIrq/hwEnableIrq. */
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

    /* SetupVolumeAndPan + the inlined CheckOutputMode: keep the requested pan
       as orgPan/orgSPan, fold the effective one for the output mode (bit0 =
       mono -> centre, no surround; bit1 clear -> no surround). */
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

/* Per frame, from the menu. Retires the in-flight read, then issues the next
   one into the ring's free region: what the DSP has PLAYED (behind hwGetPos)
   and the engine has CLAIMED (behind the fresh data). Both bounds matter --
   the play head, because the engine samples the ring's byte 0 as the loop
   predictor and a frame written there ahead of the wrap would mismatch the
   ARAM copy the DSP loops into; the claim, because unclaimed bytes are still
   waiting to be flushed. The song loops. */
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
    /* The floor judges the TOTAL free space, before the contiguity clip
       below. Judging the clipped length deadlocked: once a read ended 256
       bytes short of the ring's end, that tail could never reach the floor
       and never grew, and the engine sat waiting on it forever. A short tail
       read is fine -- the next frame carries on from offset 0. */
    if (free < LS_READ_MIN)
        return;

    len = free;
    if (s_lsFill + len > LS_RING_BYTES)
        len = LS_RING_BYTES - s_lsFill;                     /* contiguous only      */
    if (len > LS_READ_MAX)
        len = LS_READ_MAX;
    if (len > LS_SONG_BYTES - s_lsSongPos)
        len = LS_SONG_BYTES - s_lsSongPos;

    /* The claimed region's ARAM upload was queued from the audio interrupt;
       let it drain before the DVD overwrites its source. */
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
