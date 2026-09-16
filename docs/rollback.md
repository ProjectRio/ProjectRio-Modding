# Rollback seed test: is the logical seed complete?

Used by: `Gecko Codes/Match/Rollback Seed Test.c` (diagnostic, not a shipping
feature; leave it disabled except when running the test).

## The question

For MSSB rollback: is the ~18 KB logical seed (the replay-snapshot structs +
RNG + play counters) complete enough to restore at an arbitrary mid-play
frame? Hook: C2 at `simulate1FrameOfTheGame` (`0x80699D34`), once per frame,
before the logic step.

## Method

A 3-frame state machine, one step per real frame, no re-entrancy:

| mode      | action                                                      |
| --------- | ----------------------------------------------------------- |
| armed     | save the 21-region seed to SCRATCH; sim runs -> N+1 (real)  |
| waitReal  | HASH_A = checksum(band); restore the seed; sim runs -> N+1 (replay) |
| waitRepl  | HASH_B = checksum(band); RESULT = (A == B)                  |

Trigger: hold Z on P1 during a live ball (ball in flight after contact) and do
not touch the stick for ~3 frames (both sim passes must see identical inputs).

The seed (19 replay-snapshot structs + RNG + play counters):

    {0x8089298C, 0x158}, {0x80892968, 0x024}, {0x808928A0, 0x0C8}, {0x80890B38, 0x1BF8},
    {0x808909C0, 0x178}, {0x80890910, 0x0B0}, {0x80892AE4, 0x0BC}, {0x80892750, 0x150},
    {0x80892730, 0x020}, {0x8088F368, 0x15A8},{0x8088EE18, 0x550}, {0x803537E4, 0x2AC},
    {0x803535C8, 0x21C}, {0x80892BAC, 0x014}, {0x80892F8C, 0x2F4}, {0x80893280, 0x080},
    {0x80893300, 0x00E}, {0x80893310, 0x004}, {0x80893314, 0x006},
    {0x80892684, 0x020},   RNG state (the snapshot omits it)
    {0x8088A808, 0x008},   playFrameCounter + lastPlayFrameCounter

Checksum band: `0x8088A7E4` .. `0x80893AA0`, `h = h*33 + word`. SCRATCH:
`0x815B4000`, verified-free high MEM1 (a 2 MB zero run), not a game buffer.

## Results in claimed RAM

Watch these in Dolphin's memory viewer, or a savestate for offline reading
(add `0x802EBFC0..0x802EBFD4` to ClaimedFreeMemory.h):

| address      | name      | meaning                                              |
| ------------ | --------- | ---------------------------------------------------- |
| `0x802EBFC0` | HEARTBEAT | increments every frame the hook runs (proves it runs) |
| `0x802EBFC4` | MODE      | 0 idle, 1 armed, 2 waitReal, 3 waitReplay             |
| `0x802EBFC8` | RESULT    | 0 none, 1 COMPLETE (seed sufficient), 2 INCOMPLETE    |
| `0x802EBFCC` | HASH_A    | checksum of the real next frame                       |
| `0x802EBFD0` | HASH_B    | checksum of the replay next frame                     |

How to read it:

- HEARTBEAT climbing every frame => the C2 hook works. If it stays 0, the hook
  itself is broken (the crash is the injection, not the test).
- Hold Z during a live ball: MODE steps 1 -> 2 -> 3 -> 0 over three frames.
- RESULT 1 (COMPLETE) => the 18 KB seed reproduced the frame; a struct-copy
  rewind ("Path B") works. RESULT 2 with HASH_A != HASH_B => the seed is
  missing state; widen SEED[] (or the derived heap) and re-test.
- If it still crashes with the heartbeat climbing, the crash is `seed_copy` or
  the restore: that itself is the finding (a naive struct-copy rewind is not
  safe mid-play; the derived actor/animation heap holds live references).

## Changelog

- v3: no ScreenText. The earlier version crashed before the test even ran (the
  savestate showed the scratch untouched and the fault inside the text
  engine), so a C2-driven ScreenText was the culprit. Results now go to
  claimed RAM instead.
