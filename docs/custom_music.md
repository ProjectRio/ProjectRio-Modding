# Custom Music: the stream slots, the menu theme, and the Dictionary reroute

Used by: `RioModPack/Custom Music.c`, `RioModPack/MusicConfig.h`,
`Include/Rio/DictionaryReroute.h`, `RioModPack/RioModPack.c`.

Related: `docs/musyx_stream.md` (the Letters track, MusyX's own stream engine),
`docs/menu_music.md` (the Dictionary-replaces-menu-music swap: fx 484, the
moving FX table, the fade path), `docs/mod_options.md` (how the pack wires the
music config into the Options menu).

## Why it is a pack file, not a standalone gecko code

Custom Music has sixteen settings and no on/off, so it is only useful with a
UI in front of it (the RioModPack Options menu). Shipping it as a lone ini code
would give sixteen words of claimed RAM and no way to reach them. `build_all.py`
scans `Gecko Codes/` only, so keeping it under `RioModPack/` also keeps it out
of the standalone sweep. The slots are plain claimed RAM, so an ini code can
still set them if someone wants the mod without the menu.

It was originally "Menu Music Track Select", which picked ONE menu track at
build time through an `MMT_TRACK` #define, with the Dictionary swap as a third
compile-time branch.

The code is declared with NO `.state` on purpose, so it runs in every state. A
`.state = MSSB_MENU` code stops the moment the game REL loads, which would leave
the menu stream running into the match with nothing left to shut it off.

## The track model

Track ids are a flat list so the UI can step through them. The order IS the
order the Options menu steps through, and slot words hold these ids, so
renumbering re-points a configured slot; `MUSICCFG_MAGIC` is bumped with every
renumbering so a stale config from an older layout re-initialises to Default.

| id     | meaning |
|--------|---------|
| 0      | `MUSIC_DEFAULT`: leave the game's own music alone |
| 1..15  | the game's own streams; track id N -> stream id N-1 |
| 16, 17 | `star_01` / `star_03`: on the disc, unreachable by the stock game |
| 18     | `MUSIC_LETTERS`: the unused song in ZZZZ.dat (our own MusyX stream, menu only) |
| 19     | `MUSIC_DICTIONARY`: the Dictionary theme (a Musyx FX, menu only) |
| 20..29 | `snd/my_snd_h/custom_01_h.adp` .. `custom_10_h.adp` |
| 30     | `MUSIC_OFF`: no music at all |

Grouped by provenance: the game's own streams, then what ships on the disc but
is never played, then the user's own files, then Off (which also puts Off one
step from Default across the wrap).

### Slots

Slot 0 is the menu. Slots 1..15 are EVERY streamed track the game has, one per
stream id, not just the stadiums: replay, results, victory and challenge tracks
are swappable for the same cost as Mario Stadium. Slot i (i >= 1) drives stream
id i-1; the menu slot has no stream id (parked on 0xFF in `s_musicSlotStream`).

| slot | stream id | label |
|------|-----------|-------|
| 0  | -  | Menu |
| 1  | 0  | Mario Stadium |
| 2  | 1  | Bowser Castle |
| 3  | 2  | Wario Palace |
| 4  | 3  | Yoshi Park |
| 5  | 4  | Peach Garden |
| 6  | 5  | DK Jungle |
| 7  | 6  | Replay |
| 8  | 7  | Results |
| 9  | 8  | Victory |
| 10 | 9  | Toy Field |
| 11 | 10 | Challenge Map |
| 12 | 11 | Demo |
| 13 | 12 | Ending Jingle |
| 14 | 13 | Staff Roll |
| 15 | 14 | Home Run Jingle (`home_in`) |

Track labels are kept to 15 glyphs because the Options menu draws them in a
fixed column. Labels avoid `_` because the font has no underscore glyph (it
renders as `?`), which is also why the `.notes` string never spells out
`custom_01_h.adp`.

### Claimed RAM (see `ClaimedFreeMemory.h`)

| address | size | use |
|---------|------|-----|
| `0x802EB540` | 16 words | `MUSICCFG_BASE`: one word per slot |
| `0x802EB580` | 4 | `MUSICCFG_MAGIC_ADDR`: one-shot init sentinel, `'MUS4'` = `0x4D555334` |
| `0x802EB590` | 15 x 8 | `MUSIC_SAVED_BASE`: each stream's stock {path,size}, captured once |
| `0x802EB610` | 15 x 32 | `MUSIC_PATHBUF_BASE`: one path buffer per stream slot |
| `0x802EC2B0` | 4 | `g_mmtStarted`: `== MMT_MAGIC (0x4D4D5401)` while the menu stream runs |
| `0x802EC2BC` | 4 | `g_mmtTrack`: the track currently streaming on the menu |
| `0x802EC2C0` | 32 | `MMT_PATH_BUF`: the menu's path string |

`g_mmtStarted` holds a magic value rather than a flag because claimed RAM
contains whatever was there at power-on; acting on a garbage "started" would
stop a stream we never began and write a garbage pointer into the stream table.

The config is plain RAM, so every read clamps (`MusicSlotTrack`): a stale word
or a value an ini code wrote reads as Default rather than indexing off the label
table or asking the jukebox for a stream that does not exist.

## Two completely different mechanisms

### Stadiums: retarget the game's own stream descriptors

`playStream(id)` @ `0x8006877C` is table driven. The descriptor table is at
`0x800E87B4`, 16 bytes per stream id, 15 ids:

| offset | field |
|--------|-------|
| +0  | `char* path` |
| +4  | `u32 size` |
| +8  | `u32` |
| +12 | `u32 size` (again) |

The path is resolved through the disc FST at RUNTIME: `0x800A7544` does
`DVDConvertPathToEntrynum`, then `0x800A750C` does `DVDFastOpen`. Nothing about
a filename is baked into the game, so pointing Mario Stadium's descriptor at
another path is enough to change what plays when the game asks for its own
stadium music. The call is never intercepted. That is also what makes the two
unused tracks reachable (they ARE in the FST, they simply have no descriptor)
and what makes a user-added file reachable.

The two size fields turn out not to matter: the descriptor pointer is stashed
at `workBuffer+0x4C` and never read back, and every read goes through
`DVDReadAsyncPrio`, which clamps against the length `DVDFastOpen` copied out of
the FST. They are filled in anyway from that same FST so a retargeted entry
never holds a size that contradicts its path.

Applying is idempotent: each slot is recomputed from the config and the
captured original and written only when it differs, so it can run every frame
and self-corrects when the user changes a slot. It only runs in the menu
(`rel == 4`): retargeting mid-match would be read by whatever the game starts
next, and the match has already asked for its music by then.

The stock descriptors are captured once per console session
(`musicInitOnce`, guarded by `MUSICCFG_MAGIC`) BEFORE the first write.
Re-saving after a retarget would record our path as the stock one and the slot
could never be put back. "Bowser Castle plays Mario Stadium's music" must mean
the real `mario_01`, so `musicStockDesc` reads a retargetable id's ORIGINAL
from the saved copy, never its live entry.

### The menu: borrow a descriptor

The main-menu theme is a Musyx FX (fx 484), not a stream, so none of the above
applies. The menu slot BORROWS one descriptor, `home_in` (`HOST_ID` 14, a short
jingle nothing needs during the menu), points it at the chosen track, starts a
stream by hand with `playStream(14)`, and silences fx 484 separately via
`sndFXStop` @ `0x800C832C` on the handle at `0x803C6714`.

It always borrows, even for one of the game's own tracks, so the menu is
unaffected by whatever the stadium slots did to their descriptors: menu = Mario
Stadium and Mario Stadium = custom track must give two different songs.

`home_in` is also slot 15 now that every stream is configurable, so the two
cooperate: `musicApplyStreams` skips the host slot while the menu stream runs
(rewriting a descriptor under a playing stream would leave the jukebox holding
one that no longer describes what it opened), and `mmtStop` re-applies slot 15
from the config as it hands the descriptor back, so the slot is never stale for
longer than the menu stream lasts, and a match only starts after that handover.
Restoring a *saved* copy instead would quietly undo the user's Home Run Jingle
setting every time the menu stream stopped.

### The FST probe

A missing file is detectable before anything is disturbed: `MusicProbePath`
returns 0 when `DVDConvertPathToEntrynum` (`0x8007791C`) gives -1. Every path
is probed BEFORE the stock music is stopped, so a slot pointed at a file this
disc does not carry leaves the game exactly as it was rather than stopping the
stock theme and starting nothing.

The probe is pure RAM work: the FST (`*(u32*)0x80000038`, `BootInfo->FSTLocation`)
is resident from `DVDInit` onwards, so it is safe from a per-frame hook. FST
entries are 12 bytes `{ u32 isDirAndStringOff; u32 pos; u32 len }`, the top byte
of the first word marking a directory; entry 0 is the root and its length is
the entry count.

Both halves check availability (the UI's `MusicTrackStep` skips absent files so
a custom slot with nothing behind it cannot be chosen; the mod probes again
before committing) because the config is plain RAM and a value can arrive from
an ini code or a stale byte, not just from the menu. The music screen draws a
slot red when its configured track is not on the disc.

`MUSIC_OFF` on a stadium slot points the descriptor at a path that is
deliberately absent (`snd/my_snd_h/off_h.adp`): `jukeboxPlay` bails out when
`DVDFastOpen` fails (`0x800A8E74`), so nothing is queued or prepared and the
match is silent. On the menu slot the Musyx theme is stopped and nothing is
started in its place.

## The jukebox

| address | what |
|---------|------|
| `0x8006877C` | `playStream(u8 id)` |
| `0x800A86B4` | `jukeboxCmd(u32)`; 4 = cancel the DVD stream |
| `0x800A8F68` | `jukeboxStop()`: the hard reset, the same call SND init makes at `0x80021A18`; zeroes head/cur/tail |
| `0x8034E478` | `JUKEBOX_WORK`: 80 bytes per stream id; the descriptor pointer is at work+0x4C |
| `0x803CC150` | `JUKEBOX_HEAD` (r13 - 0x75F0) |
| `0x800A8964` | the state==2 resume path: re-triggers with no fresh prepare |
| `0x800A8E74` | `jukeboxPlay` bail-out when `DVDFastOpen` fails |

### The queue trap

The jukebox is a QUEUE, not a single slot: `jukeboxPlay` links a new work
buffer in behind whatever is already going (`cur->next` at +4, prev at +0) and
only starts it when the list was empty. When the match loads and game.rel asks
for the stadium track while our menu stream is still running, its request is
parked BEHIND ours and the menu music keeps playing. Worse, `jukeboxStop` throws
the parked request away, so the stadium track never arrives at all.

`mmtQueuedGameStream` walks the queue before the reset and picks up whatever
the game asked for (ids come out of the descriptor pointer at work+0x4C), so
`mmtStop` can re-issue it with `playStream` afterwards.

### The DTK trap

The `.adp` tracks are hardware DTK: `DVDPrepareStreamAsync` points the DRIVE at
a disc region, and `jukeboxStop` only calls `AISetStreamPlayState(0)`, which
stops playback but leaves the drive pointed at our track. Whoever next asks the
jukebox to resume gets a re-trigger with no fresh prepare (the state==2 path at
`0x800A8964`) and the menu track comes back mid-match. `jukeboxCmd(4)` is the
game's own cancel: it zeroes the stream volumes and issues
`DVDCancelStreamAsync` with the jukebox's own command block, so the drive holds
nothing and the next `playStream` has to prepare the track it actually wants.
Cancel FIRST, then stop.

### Stale head on the way back from a match

The game stops a stream by dropping the jukebox to state 0 and leaves it
LINKED, so coming back from a match the stadium track is still head (measured:
`w0:mario_01_h.adp -> w13:cha_s_roll_h.adp`). Appending behind it means the
stale track resumes and ours never starts, so `mmtStart` calls `jukeboxStop`
before `playStream`.

## The menu-music routine and its guard

The stock routine at `0x80062A94` plays fx 484 via `sndFXStartEx(484)` and
keeps its handle at `0x803C6714` (`g_menuMusicHandle`; `0xFFFFFFFF` on a failed
start, e.g. the bank is not loaded yet) with an "already playing" guard u8 at
`0x803C6718` (`g_menuMusicGuard`). While our stream plays the guard is held at 1
so the routine never restarts fx 484 underneath it; only while a stream really
got going (on a missing file `mmtStart` stands down and the stock theme must
keep playing).

`mmtStop(releaseGuard)`: the guard is released only when the game REL takes
over or the user goes back to Default/Dictionary. Muting for the Dictionary
scene (screenCode 7) must keep fx 484 suppressed, or it restarts underneath
that scene's own track, which is exactly the two-songs-at-once bug that scene
is known for.

`rel` (`0x800E877C`: 0 = boot, 4 = menu, 5 = game) is a #define from
GlobalData.h, so it must not be shadowed with a local. `screenCode` is the u16
at `0x800E877E`.

### Per-frame decision order (menu, `rel == 4`)

1. Apply the stream slots.
2. If the selection is not Letters and Letters is active, stop it (Default and
   Dictionary also hand the guard back).
3. Dictionary selected: stop any host stream with the guard released; the
   Dictionary swap (included in the pack, see `docs/menu_music.md`) works
   THROUGH the stock routine and needs the guard back to restart its track.
   Checked before the screenCode 7 case so the guard is never held in this mode.
4. screenCode 7 (Dictionary scene): stop the host stream and Letters, keep the
   guard held (that scene has its own track).
5. Off: retire whatever plays, stop the handle, hold the guard.
6. Letters: stop the host stream (guard stays), silence fx 484, start the MusyX
   stream, hold the guard while it is active, pump the ring each frame.
7. Default: stop ours, release the guard so `0x80062A94` restarts fx 484.
8. Track change while playing: stop this frame, start next frame.
9. Otherwise start the host stream and hold the guard while it is running.

## The Dictionary reroute (`Include/Rio/DictionaryReroute.h`)

`DictionaryReroute_Tick()` is called once per frame from a per-frame
`.state = MSSB_MENU` code.

1. **Reroute.** The main-menu dispatch at `0x80641848` is
   `li r3,8; bl changeScreenVariables` (Records, screenCode 8). Writing
   `li r3,7` (`0x38600007`) over it every frame while the menu REL is resident
   makes the Records button open the unused Dictionary scene (screenCode 7).
2. **Music fix** (reverse-engineered live, 2026-07-24). The stock Dictionary
   scene starts its own track without stopping the menu voice, so two songs
   layer. On ENTERING Dictionary (`screenCode` edge to 7, read from
   `*(u16*)(*(u32*)0x803CBBCC + 2)`) we call `synthKillAllVoices(0)` @
   `0x800D15FC` and set the handle to `0xFFFFFFFF` so the resume logic cannot be
   fooled by the stale pre-kill handle. On LEAVING we arm a "resuming" latch;
   back on the main menu (screenCode 5) we clear the guard each frame until a
   real handle appears (right after leaving, the menu bank is still reloading,
   so `sndFXStartEx` can fail), then stop, because clearing further would replay
   on top and double the music.

Claimed RAM: `0x802EC280` (u16, last frame's screenCode) and `0x802EC284`
(u32 latch, `0x0D1C7` while resuming).

## Pack wiring notes

- The pack includes `Gecko Codes/Menu/Dictionary Replaces Menu Music.c` with
  `CGECKO_ACTIVE` defined as `MusicSlot(MUSIC_SLOT_MENU) == MUSIC_DICTIONARY`.
  It is NOT gate-wrapped: it edits game state (fx 484's layer id and the menu
  music volume) and must keep running while off to put those back.
- `DMM_SCREENS(sc)` adds the Options screen (6) and the Online screen to the
  main menu (5). Without it the swap does not happen until the user backs out,
  while every other track swaps the moment it is selected.
- Custom Music itself is not gated either (gating it away would stop it handing
  the audio back when a match loads); only `CGECKO_OPTION_ADDR` is set so the
  Options row for `MODOPT_MUSIC` can find its notes.
