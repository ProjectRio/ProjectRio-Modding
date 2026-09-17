# Reviving MusyX's stream engine: the "Letters" song

Used by: `RioModPack/LettersStream.h` (included by `RioModPack/Custom Music.c`
and driven only from its `CustomMusic()` hook: cgecko links every hook
separately, so a static touched from two hooks would exist twice).

Related: `docs/custom_music.md`.

## The song

`ZZZZ.dat +0x08F2E808` holds an AdGCForm blob nothing references: an 8-byte
magic + 0x20 wrapper, a standard 0x60-byte DSP-ADPCM header, then sample data
at `+0x88`. Mono, 32000 Hz, 9,226,688 samples (4:48, fades out).

It is NOT the raw DTK ADP the stadium themes stream as (a DTK block repeats
bytes 0/1 at 2/3; this never does), so no stream descriptor can play it, but it
is exactly the sample format MusyX plays.

| constant | value |
|----------|-------|
| `LS_SONG_BLOB` | `0x08F2E808` (offset within ZZZZ.dat) |
| `LS_SONG_DATA` | blob + 0x88 |
| `LS_SONG_BYTES` | `0x507340` (5,272,392 floored to whole 32-byte frames) |
| `LS_SONG_FRQ` | 32000 |
| coefficients | 16 x s16 at blob +0x44, copied verbatim into `kLettersCoef` |
| initial predictor/scale | `data[0]` |

## The engine

`streamHandle` (`0x800C8874`) is whole in the retail DOL and runs every few
audio frames from `hwHandle`; only the public setup calls
(`sndStreamAllocEx` / `sndStreamActivate` / `voiceBlock` /
`aramAllocateStreamBuffer`) were dead-stripped, so nothing ever hands it a
stream and it idles. LettersStream.h is that setup, transcribed from the
decomp: an ARAM ring, a BLOCKED voice, `streamInfo[voice]` filled in, and the
game's own code does the rest. Every struct offset was checked against the
retail disassembly.

### STREAM_INFO

`0x8030F970`: `STREAM_INFO[64]`, stride `0x68`, indexed BY VOICE.

| offset | field |
|--------|-------|
| 0x00 | next (paired stereo stream; `0xFFFFFFFF` = none) |
| 0x04 | stId (any non-zero id; we use `'LETT'` = `0x4C455454`) |
| 0x08 | flags (bit0 = ADPCM) |
| 0x0C | state (u8; 1 = hand to the engine, 0 = free) |
| 0x0D | type (u8; 1 = ADPCM) |
| 0x0E | hwbuf (u8; ARAM stream-buffer index) |
| 0x10 | updateFunction |
| 0x14 | buffer (RAM ring) |
| 0x18 | size (SAMPLES) |
| 0x1C | bytes |
| 0x20 | last |
| 0x24 | SNDADPCMinfo: u16 numCoef, u8 initialPS, u8 loopPS, s16 y0, s16 y1, s16 coef[16] |
| 0x4C | voice |
| 0x50 | user |
| 0x54 | frq |
| 0x58 | prio (u8) |
| 0x59 | vol |
| 0x5A | pan |
| 0x5B | span |
| 0x5C | auxA |
| 0x5D | auxB |
| 0x5E | orgPan |
| 0x5F | orgSPan |
| 0x60 | studio |

Pan/span follow `SetupVolumeAndPan` + the inlined `CheckOutputMode`: keep the
requested pan as orgPan/orgSPan, fold the effective one for the output mode
(`synthFlags` `0x803CC274`: bit0 = mono -> centre, no surround; bit1 clear ->
no surround).

### The voice block

`SYNTH_VOICE[]` is reached through the pointer at `0x803CC278`, stride `0x458`;
the voice count is the u8 at `0x8030E7D8` (`synthInfo + 0x210`).

| offset | field |
|--------|-------|
| 0x34  | addr (`MSTEP*`) |
| 0xF4  | id (u32) |
| 0x100 | allocId (u32: `mcmdPlayMacro` loads it with `lwz` and passes it through, which a u16 would never compile to) |
| 0x11C | block (u8) |
| 0x11D | fxFlag (u8) |

`LettersVoiceBlock` is a transcription of the stripped `voiceBlock()`
(synthvoice.c), which is what `sndStreamActivate` really uses, NOT a bare
`voiceAllocate`. `block=1` keeps voice stealing from handing this voice to the
next SFX; the sentinel id (`v | 0xFFFFFF00`) / allocId (`0xFFFF`) keep the
by-id lookups from ever matching it. `voiceAllocate` returns the RAW index (or
`0xFFFFFFFF`).

### The ARAM stream-buffer pool

64 x `{next, aram, length, allocLength}` at `0x80326938`, stride 0x10, offsets
read back out of `aramGetStreamBufferAddress`. Lists: idle `0x803CC3A8`, used
`0x803CC3B0`, the downward-growing ARAM cursor `0x803CC3BC`.

`LettersAllocAram` is the "carve a fresh one off the top" path of the stripped
`aramAllocateStreamBuffer()` only; nothing else allocates these, so the free
list is always empty. The handle is kept for the session (the free path was
stripped and 8 KB of ARAM is nothing).

### Retail entry points

| address | function |
|---------|----------|
| `0x800D0B00` | `voiceAllocate(prio, 0xFF, 0xFFFF, block)` |
| `0x800D10EC` | `voiceUnblock` (what `sndStreamDeactivate` does) |
| `0x800CFFF8` | `vidRemoveVoiceReferences` |
| `0x800D052C` | `voiceSetPriority` |
| `0x800CF97C` | `macMakeInactive` (state 2 = stopped) |
| `0x800DC97C` | `hwIsActive` |
| `0x800DCB6C` | `hwBreak` |
| `0x800DD3DC` | `hwGetPos` |
| `0x800DE778` / `0x800DE740` | `hwDisableIrq` / `hwEnableIrq` |
| `0x800DDA1C` | `aramSyncTransferQueue` |
| `0x8006E868` | `DCInvalidateRange` |

Raw pointers carry an `LS_` prefix so nothing collides with the musyx header
inlines Custom Music.c already pulls in.

## The refill contract

Read out of `streamHandle`'s state-2 path:

```
cpos = hwGetPos(voice) rounded down to a whole ADPCM frame;
if (last != cpos) ret = updateFunction(buf1, len1, buf2, len2, user);
last = (last + ret) % size; hwFlushStream(the claimed bytes) -> ARAM.
```

Lengths are SAMPLES, the pointers arrive pre-offset, and a short or zero return
is handled cleanly. After each refill the engine reads the ring's byte 0
(uncached) as the loop predictor, so byte 0 must always be a frame header and
must never be rewritten ahead of the play head.

The callback (`LettersFeed`) is called from the audio interrupt. It only reports
how much data is already there, in whole 32-byte units, never more than the
contiguous `len1`, so the engine always takes its single-flush path and byte
cursors stay aligned. Its address is stored in streamInfo, which needs CGecko
>= 70ae670 (before that `&function` pointed inside the payload).

## Zero-copy ring

One 8 KB ring (`LS_RING_BYTES`), 14336 samples, shared: DVD reads land directly
in the region the DSP has already played and the engine has already flushed,
and the callback just reports how much of that has arrived. Claims are made in
56-sample (32-byte) units so the engine's byte cursor stays 32-byte aligned for
the DVD.

The ring is a static array in the mod's image, 32-byte aligned as
`hwFlushStream` and `DVDRead` require. In the DOL-baked pack that is free: the
image lives in a reserved region (`0x817C0000`, ~253 KB, a few percent used).
In a gecko build it costs 8 KB of zeros in the code list.

State (all zero at load): `s_lsPhase` (0 idle, 1 priming, 2 running),
`s_lsVoice`, `s_lsHwbufPlus1`, `s_lsReadBusy`, `s_lsReadLen`, `s_lsFill` (ring
byte offset the next read lands at), `s_lsSongPos`, `s_lsAvail` (fresh bytes
ahead of the engine's cursor, written by the callback and the pump, so the pump
edits it with IRQs off), `s_lsFile` (a `DVDFileInfo`).

### Phases

- **Start**: `DVDOpen("ZZZZ.dat")` (FST lookup only), invalidate the ring, issue
  one async read of the whole ring from `LS_SONG_DATA`, phase 1.
- **Prime** (phase 1 -> 2 once that read lands): allocate the ARAM buffer,
  block a voice at priority 0x7F, zero and fill `streamInfo[voice]`, set state 1
  under `hwDisableIrq`/`hwEnableIrq` (the bracket `sndStreamAllocEx` uses). The
  engine builds the SAMPLE_INFO, starts the voice and flushes the whole ring to
  ARAM on its next service. `s_lsAvail` starts at 0, NOT `LS_RING_BYTES`:
  the engine flushes the primed ring itself (state 1), so nothing in it is
  "fresh, unclaimed". Calling it so made the engine re-flush lap 1 as lap 2 (the
  first half second played twice) and threw the pump's accounting off by a lap.
- **Pump** (per frame from the menu): retire the in-flight read, then issue the
  next one into the ring's free region, i.e. what the DSP has PLAYED (behind
  `hwGetPos`) AND the engine has CLAIMED (behind the fresh data). Both bounds
  matter: the play head because of the byte-0 loop predictor, the claim because
  unclaimed bytes are still waiting to be flushed. `aramSyncTransferQueue` runs
  before the DVD overwrites a region whose ARAM upload was queued from the
  audio interrupt. The song loops.
- **Stop**: cancel an in-flight read ASYNCHRONOUSLY and leave it marked busy
  (the next Start waits for the command block to go idle), `voiceUnblock`, state
  0. The file is never closed: `DVDClose` is bookkeeping-free on this SDK and a
  later `DVDOpen` refills the same `DVDFileInfo`.

Read sizing: `LS_READ_MIN` 1024 (do not bother below), `LS_READ_MAX` 4096 (one
DVD request), priority 2. The floor judges the TOTAL free space, before the
contiguity clip. Judging the clipped length deadlocked: once a read ended 256
bytes short of the ring's end, that tail could never reach the floor and never
grew, and the engine sat waiting on it forever. A short tail read is fine; the
next frame carries on from offset 0.

Cache: invalidate the target region before every read. `hwFlushStream` will
`DCStoreRange` the region before its ARAM DMA, and a stale line would write old
bytes back over the fresh read.

## Crash lessons

### Nothing here may sleep

Gecko hooks run with `OSCurrentThread == NULL`, so any SDK call that parks the
caller (the synchronous `DVDReadPrio`, `DVDCancel`) faults writing
`thread->state` ("Invalid write to 0x000002c8" in `OSSleepThread`). Every disc
operation is the Async form, polled by the pump, and priming the ring is a
phase of its own rather than a blocking read.

### The render thread's stack at 0x802ED140

The ring was briefly `lbl_802ED140`, a "discrete 8192-byte .bss object
referenced by nothing", which turned out to be the RENDER THREAD'S STACK
(OSThread `0x803C7488`, priority 12: stackEnd `0x802ED140`, stackBase
`0x802EF140`). The priming read overwrote it and the thread returned into
garbage. Every reference scan missed it because a stack's only reference is to
its END, `0x802EF140`, which is the address of the NEXT object. Do not go
looking for "free" game RAM for a DMA target by reference-scanning again: dump
it live on an unmodded boot (the repo's actual standard) or use the image.
