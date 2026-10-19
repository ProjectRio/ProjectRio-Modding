# Menu music: how the main-menu theme is chosen, and swapping it

Used by: `Gecko Codes/Menu/Dictionary Replaces Menu Music.c` (also one of the
tracks in `RioModPack`'s music config).

## How the menu music actually works (reverse-engineered live 2026-07-24)

The menu BGM is started by the routine at `0x80062A94` as `sndFXStartEx(484)`.
An FX entry is 10 bytes:

| offset | field     |
| ------ | --------- |
| +0     | u16 fxId  |
| +2     | u16 objId |
| +4     | u8 prio   |
| +5     | u8 maxVoices |
| +6     | u8 vol (the entry's authored volume) |
| +7     | u8 pan    |
| +8     | u8 key    |
| +9     | u8 vel    |

The objId's top two bits are a type tag, not a flag: `0x0000` = macro,
`0x4000` = keymap, `0x8000` = layer (see `0x800C54C8`'s
`rlwinm r4,objId,0,16,17`). So:

| fx  | objId  | what                                   |
| --- | ------ | -------------------------------------- |
| 484 | 0x8022 | layer 0x22, the menu theme, in group 32's pool |
| 0   | 0x8021 | layer 0x21, the Dictionary theme, in group 0's pool |

Group 32 is loaded on the menu; group 0 is not. It only arrives when the
Dictionary scene loads audio file set 4. Simply pointing fx 484 at layer 0x21
from the menu makes `sndFXStartEx` return -1 (verified) and the menu goes
silent, because the layer is not in any loaded pool. So the swap has to:

1. get audio file 4 loaded and its group 0 pushed, and only then
2. repoint fx 484 at layer 0x21 and restart the track.

### The loaded-group array

`fxGroupCount` u16 at `0x803CC2D8`; `FX_GROUP_ARRAY` at `0x80316D70`, 12
bytes per entry: `{ u16 groupId; u16 numFx; u32; u32 fxTable; }`. The FX table
*moves*: leaving a scene reloads the sound files to a new address (seen:
`0x80A84DF0` -> `0x80A862F0`). Never cache the entry pointer; re-find it
through the loaded-group array every time. The array is rebuilt on a screen
change and reads empty for a few frames either side of one, so a "group gone"
probe has to be debounced (the code requires 30 consecutive missing frames).

### The loader task (0x80021758)

Audio loading is a task node the game already knows how to run: task fn
`0x80021758`, allocated with `insertTask` (`0x800B0A5C`), fields
`+0x0C` = owner node (insertTask sets it to the current node; the code repoints
it at scratch `0x802EC290` so the "done" write lands somewhere harmless and
doubles as a completion flag at `+0x10`), `+0x14` = push callback, `+0x18` =
state, `+0x19` = audio file index. It walks state 0 (start the load via
`0x800A70DC`) -> 1 (wait for the DVD, then relocate the descriptor) -> 2 (call
the push callback, flag the owner, deregister itself). The Dictionary's own
node is pool entry 20, `{ 0x800627C4, state, id 4 }`; `0x800627C4` is the
game's `pushSoundGroup(0, *(0x800EF81C))` for that file set.

State 0 stores the loaded buffer at `0x800EF808 + fileIndex*4 + 4`, so file 4
lands at `0x800EF81C`, which is exactly the word the push callback re-reads.

A loaded audio file is a 0x20-byte header of four `{ u32 off; u32 size; }`
section records (prj, sdir, pool, samples) followed by the sections, so the
first one always begins 0x20 past the header.

### The double-relocation crash

The loader task's state 1 relocates the descriptor in place and
unconditionally:

    d = *(0x800EF81C);
    *(d+0x00) += d;  *(d+0x08) += d;  *(d+0x10) += d;  *(d+0x18) += d;

Nothing marks the descriptor as done, so running the task a second time for a
file that never left RAM adds the base again and every section pointer becomes
`2*d + off`. State 2's push callback re-reads `0x800EF81C` and hands that to
`sndPushGroup`, whose first act is `while (g->nextOff != 0xFFFFFFFF)` through
the wild pointer. Measured from the crash savestate: d = `0x8110F1A0`, prj
went 0x20 -> `0x8110F1C0` (correct) -> `0x0221E360` (= 2*d + 0x20), "Invalid
read from 0x0221e360, PC = 0x800d3440" (inside `sndPushGroup`, `0x800D3120 +
0x320`).

The three values of `*(d+0)` are all distinguishable, which is the descriptor
classifier the code uses:

| `*(d+0)`     | state                                          |
| ------------ | ---------------------------------------------- |
| 0x20         | loaded, relocation still to come (a task is mid-flight) |
| d + 0x20     | relocated exactly once: safe to push directly (`0x800627C4`) |
| 2*d + 0x20   | relocated twice, already ruined; leave it alone |

When the group has been popped but the file is still resident and relocated
once, push it straight back rather than queueing a new loader task.

### Restart and volume traps

- Do not restart the music with the fade u8 at `0x803C671A`. That is the stop
  path: it calls `sndFXStop` and then `0x800B0A14`, which deregisters the
  music updater, leaving the menu permanently silent. Restart by stopping the
  handle at `0x803C6714` (`sndFXStop`, `0x800C832C`) and clearing the
  "already playing" guard u8 at `0x803C6718`; the routine re-reads the FX
  entry on every start.
- The menu music routine starts the track at the *menu's* music volume
  (`lbz r4,-32440(r13)` at `0x80062AD0`, u8 at `0x803CB888`, 105 on a stock
  boot), not the FX entry's default of 127. The entry default only applies
  when the caller passes 255, which this routine never does, so the Dictionary
  track ends up ~83% as loud as in its own scene. Only two instructions read
  that u8 (`0x80062AD0` start volume, `0x80062B3C` fade base), both in the
  music routine, so pinning it to the entry's authored volume affects nothing
  else. It has to be re-applied every frame: something outside those readers
  writes it back (measured: reverted 127 -> 105 between two tests).

### Which screens

screenCode 5 is the main menu, the only screen that matters on its own. A pack
that lets the user choose this track has to also run on the screen the choice
is made on, or the swap waits until they back out to the main menu (every
other RioModPack track is a stream and swaps instantly, so a delayed
Dictionary reads as broken). Widening is safe because the work is idempotent,
but not to every menu screen: an inactive screen would also start a file load
there, and pushing group 0 on a screen with its own audio set is untested.
`DMM_SCREENS(sc)` is the override point.

### Self-gating

The code self-gates on `CGECKO_ACTIVE` rather than being wrapped in
`CGECKO_GATE_ADDR`: it edits game state (fx 484's layer id and the menu music
volume), so it must keep running while off to put those back. A gate-wrapped
code is simply not executed and could never undo itself. The loaded sound
group is deliberately not unloaded on stand-down: it arrived through the
game's own loader and costs only pool space.

Claimed RAM: `0x802EC288` loading flag, `0x802EC28C` watchdog ticks (900
frame timeout), `0x802EC290` fake owner node (+0x10 = done flag),
`0x802EC2A8` saved volume (`0x100 | original`), `0x802EC2AC` consecutive
frames fx 0 has been missing.

## Retracted theories

- "Both songs are sfx 484 sharing one ARAM slot": wrong, and could never
  work; they are different layers in different groups (see above).
- "Acting on a single frame of fx 0 missing means the group is gone": wrong;
  the array is mid-rebuild on a screen change. Acting on it queued a redundant
  load for a resident file and produced the double-relocation crash.
