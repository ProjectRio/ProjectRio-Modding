# Stadium files and the stadium-select write

Used by: `Gecko Codes/Match/Stadium Asset Swap.c`,
`Gecko Codes/Menu/Nighttime Mario Stadium.c`.

## How the game picks a stadium file

Every stadium's geometry lives in ZZZZ.dat as one LZSS blob, and the DOL keeps
a table of where each blob is: `StadiumFiles` (.data `0x800EFBE8`), 7 stadiums
x 3 variants of `AssetLoadInstructions`, 0x10 bytes each.

`manageStadiumLoading` (game.rel `0x8063F588`) does the lookup in its state 0:

    0x8063F5E4  addi r0, r3, -0x418     ; r0 = 0x800EFBE8 (StadiumFiles)
    0x8063F5E0  lbz  r6, 9(r5)          ; GameInitVariables.StadiumID
    0x8063F5E8  lbz  r7, 0xA(r5)        ; .miniGameStadiumIndicator
    0x8063F5F0  mulli r3, r6, 3         ; StadiumID * 3
    0x8063F5FC  slwi  r3, r3, 4         ;   ... * 0x10
    0x8063F600  add   r3, r0, r3
    0x8063F604  bl    0x800A70DC        ; loader gets the entry, verbatim

The entry is the whole decision; nothing downstream re-derives the disk
offset. Overwrite the entry and the stadium loads whatever it points at.

The variant index is `miniGameStadiumIndicator`: 0 for a normal game, 1 and 2
for the minigame cuts of the same stadium (Bob-omb Derby and friends). Four of
the seven stadiums use one blob for all three.

### What actually changes

The blob carries the field: `StadiumFileHeader`, the models, and the collision
mesh, all of which follow the swap. What does not follow is anything derived
from StadiumID instead of from the file: `loadStadiumObjects` (`0x806F8C48`)
still spawns the original stadium's props and hazards, and
`initStadiumLighting` (`0x8001CBD4`) still picks its lighting. Expect the
destination's field with the source stadium's furniture, and expect some
combinations to fault outright (props positioned for a field that is no longer
there).

There is a second copy of this table in game.rel at `0x807B1DEC`, read only by
`FUN_80645570`. Nothing in game.rel's text branches to that function (checked
every b/bl in the section), so the DOL table is the live one.

The swap is applied per frame rather than once: `StadiumFiles` is DOL .data,
back to stock on every boot, and the write has to be in place before the match
starts. The loader only reads the entry at stadium-load time.

### The stock table (main.dol 0x800EFBE8)

`AssetLoadInstructions` = `{ const, { compressionFlag, unused, originalDiskSize },
diskLoc, compSize }`. The raw second word is given so entries can be checked
against a hex dump.

| stadium | variant | raw word 2 | origSize | diskLoc    | compSize   |
| ------- | ------- | ---------- | -------- | ---------- | ---------- |
| Mario   | 0       | 40168A6C   | 0x168A6C | 0x06CFD000 | 0x000C69A8 |
| Mario   | 1       | 401331E0   | 0x1331E0 | 0x06DC4000 | 0x000B67C4 |
| Mario   | 2       | 4011924C   | 0x11924C | 0x06E7A800 | 0x000A5764 |
| Bowser  | 0       | 400FBAE0   | 0x0FBAE0 | 0x06F20000 | 0x000BE038 |
| Bowser  | 1       | 400E5AC0   | 0x0E5AC0 | 0x06FDE800 | 0x000B4FB8 |
| Bowser  | 2       | 40131200   | 0x131200 | 0x07093800 | 0x000A317C |
| Wario   | 0,1,2   | 4011B660   | 0x11B660 | 0x07137000 | 0x000D0294 |
| Yoshi   | 0,2     | 40141C60   | 0x141C60 | 0x07207800 | 0x000CE904 |
| Yoshi   | 1       | 4013CA40   | 0x13CA40 | 0x072D6800 | 0x000CA560 |
| Peach   | 0,2     | 4016C138   | 0x16C138 | 0x073A1000 | 0x000ED41C |
| Peach   | 1       | 401157B8   | 0x1157B8 | 0x0748E800 | 0x000CA83C |
| DK      | 0,1,2   | 400EB1C0   | 0x0EB1C0 | 0x07559800 | 0x000A3510 |
| Toy     | 0,1,2   | 400D04E0   | 0x0D04E0 | 0x075FD000 | 0x00087390 |

Every entry's first word is `0x0000040B` and the compression flag is 1.

The code keeps the stock table so a swap can be undone when `CGECKO_ACTIVE`
is a runtime flag (a pack that redefines it gets a mod that can be switched
off mid-session instead of only at boot).

## The stadium-select write (Nighttime Mario Stadium)

The stadium-select screen writes the chosen stadium into the match-setup
struct:

    0x80650674  stb r5, 9(r4)      stadium id
    0x80650678  stb r0, 0x58(r3)   <- hook site, r4 still the setup struct

The byte right after the stadium id, +0xA, is the day/night flag the stadium
loader reads (0 = day, 1 = night). Mario Stadium is id 0 and the only stadium
with a night variant, so once the id is written the flag is flipped for it and
every other stadium is left alone. Nothing is patched, so a pack can gate the
whole code away with `CGECKO_GATE_ADDR` and the stock write path is untouched.
