# Netplay desync checksum

Used by: `Gecko Codes/Rio Built-in/Checksum.asm` (hook at `0x8000928C`,
re-issues `cmplwi r24, 0x0`).

Every frame the code sums a set of game-state variables into one word at
`0x802EBFB8`. Project Rio reads that word on every client and compares; if
they all agree the clients' games can be assumed to be in sync, and a
difference is reported as a desync. The sum is a plain add (with the loop
index folded in for the per-slot team values), not a hash, so it detects
divergence rather than identifies it.

## What goes into the sum

Menu state:

| address      | meaning        |
| ------------ | -------------- |
| `0x800E877E` | current scene  |
| `0x800E8782` | previous scene |
| `0x800E874C` | ports          |

Team info, for i in 0..17 (both rosters), each added with `i`:

| address      | meaning                 |
| ------------ | ----------------------- |
| `0x803C6726` | roster character ids    |
| `0x8035323B` | superstar icons         |

In-game state (one byte each unless noted):

| address      | meaning                 |
| ------------ | ----------------------- |
| `0x80892AAA` | current game state      |
| `0x80892AAB` | previous game state     |
| `0x808909A1` | contact made            |
| `0x80892857` | pickoff attempt         |
| `0x8036F3A9` | game is live            |
| `0x808909AA` | play is ready to start  |
| `0x80872540` | is replay               |
| `0x80890971` | batter roster id (added twice, once here and once further down) |
| `0x808928A3` | inning                  |
| `0x8089294D` | half inning             |
| `0x8089296F` | balls                   |
| `0x8089296B` | strikes                 |
| `0x80892973` | outs                    |
| `0x80892AD6` | P1 stars                |
| `0x80892AD7` | P2 stars                |
| `0x80892AD8` | is star chance          |
| `0x808909BA` | chem links on base      |
| `0x8088F09D` | runner on 1st           |
| `0x8088F1F1` | runner on 2nd           |
| `0x8088F345` | runner on 3rd           |
| `0x808938AD` | outs during play        |
| `0x808909A3` | hit by pitch            |
| `0x80893BAA` | final result            |
| `0x808928A4` | away team runs          |
| `0x808928CA` | home team runs          |
| `0x80890AD9` | pitcher roster id       |
| `0x80890B38` | ball pos X (word)       |
| `0x80890B3C` | ball pos Y (word)       |
| `0x80890B40` | ball pos Z (word)       |

The menu/team sections use the `Common.s` macros (`backup_nv`, `loadhz`,
`load`, `restore_nv`); the in-game section is still written out with explicit
`lis/ori/lbz` and could be moved to the macros.
