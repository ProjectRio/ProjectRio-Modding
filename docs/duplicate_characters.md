# Duplicate characters: how the draft tracks "taken"

Used by: `Gecko Codes/Menu/Duplicate Characters.c` (also `#include`d by
`Gecko Codes/Global/Instant Randoms.c`), and the dup-load model-bind hooks in
`Instant Randoms.c` / `Boot To Match.c`.

## The taken table

Character select keeps a 54-byte "already taken" table, one byte per character
id, at `Static_Stats_Tables + 0x4757` (`0x803530F7`). A non-zero byte greys
that character out. The stock code sets it from five `stb r5, 0x4757(r3)`
sites in `addRemoveCharVariantRelated` (`0x80067B40`) plus their menus.rel
counterparts.

The mod: stop the game marking anyone taken, clear whatever is already marked,
then put the mark back for just the two captains (they really are
unavailable).

| symbol        | address      | meaning                                    |
| ------------- | ------------ | ------------------------------------------ |
| TAKEN_TABLE   | `0x803530F7` | Static_Stats_Tables + 0x4757, 54 bytes     |
| CHARSEL_SLOTS | `0x803C6050` | charSelectStruct + 0x28, 36 bytes, 0xFF = empty slot |
| CAPTAIN_A     | `0x803C6726` | cursorPositions + 0x02                     |
| CAPTAIN_B     | `0x803C672F` | cursorPositions + 0x0B                     |
| MENU_SCREEN   | `*(u16*)(*(u32*)0x803CBBCC + 2)` | menuCtrl->screenCode; 9 = captainSelect (`0x8065201C`), 10 = teamSelect (`0x80650144`, the draft) |

### Patch sites

`(site, original, patched)`. The REL originals are not the same instructions
as their DOL counterparts (stwx vs sth, `cmpw r3,r0` vs `cmpw r0,r7`); they
were read out of live RAM with menus.rel resident.

| site         | original                    | patched            |
| ------------ | --------------------------- | ------------------ |
| `0x80067BAC` | `stb r5, 0x4757(r3)` 98A34757 | nop              |
| `0x80067BC8` | 98A34757                    | nop                |
| `0x80067BE4` | 98A34757                    | nop                |
| `0x80067C00` | 98A34757                    | nop                |
| `0x80067C1C` | 98A34757                    | nop                |
| `0x8004E548` | `sth r0, 0x18(r8)` B0080018 | nop                |
| `0x8004E6B0` | `cmpw r0, r7` 7C003800      | `cmpwi r0, 0xFF` 2C0000FF |
| `0x8064EC28` | `stb r7, 0x4757(r4)` 98E44757 | nop (menus.rel)  |
| `0x8064EC38` | `stb r7, 0x4757(r5)` 98E54757 | nop              |
| `0x8064ECE8` | `stb r5, 0x4757(r3)` 98A34757 | nop              |
| `0x806553F8` | `stwx r0, r30, r31` 7C1EF92E | nop               |
| `0x806553D4` | `cmpw r3, r0` 7C030000      | `cmpwi r0, 0xFF`   |

Patching uses `PatchInstruction_Conditional`, which only writes when the
expected word is there, so ON and OFF are the same call with the last two
arguments swapped, both idempotent. It re-arms itself for free: after a
menus.rel reload the original is back, the site matches again, and the next
frame re-applies. No saved state needed.

### Why per frame and not 04 writes

The original is a static patch list, fine for a code you enable in an ini and
forget. This one is a toggle, so it has to be able to put the game back, and a
REL patch has to be re-applied every time menus.rel reloads anyway. The
original gecko code, for reference:

    003530f7 00350000   zero 54 bytes at 0x803530F7   (0x35+1; the count is
    003c6050 002300ff   36 bytes of 0xFF at 0x803C6050  the HIGH halfword,
                                                        see codehandler.s
                                                        `rlwinm r10,r4,16,16,31`)
    04067bac/bc8/be4/c00/c1c 60000000   nop the five stb sites (DOL)
    0464ec28/ec38/ece8       60000000   the same three, menus.rel
    0404e548 60000000  046553f8 60000000
    0404e6b0 2C0000FF  046553d4 2C0000FF
    c264f394 ...       re-mark the two captains

The C2 the original injected at `0x8064F394` is hoisted to per-frame: the
table is only read while character select is drawing, so refreshing it each
frame is equivalent.

### The captain re-mark bug

`cursorPositions` is not cleared between visits, so on the captain-select
screen it still holds the captains chosen last time round. Re-marking those
two there greys out exactly the characters the user came back to pick again
(reported as "I cannot select my previous captains when I reenter the captain
select screen"). So the wipes stay unconditional (with duplicates on nobody is
taken until the draft says so) and only the re-marking is limited to
`MENU_SCREEN == 10`. A cursor slot reads 0xFF until a captain has been picked,
and `TAKEN_TABLE + 0xFF` is 200 bytes past the end of the table, so only ids
< 54 are ever marked.

## Duplicates and variants have chemistry

(Authors: PeacockSlayer, LittleCoaks; was the standalone code of that name.)

`Static_Stats_Tables.characterStats` is the master stat table, one row per
CHAR_ID; rows are copied into `inMemRoster` when a roster is built, so this is
the one place to edit chemistry for everyone. `chemistry` is one u8 per
CHAR_ID in id order. Every character gets max chemistry (99) with a copy of
themselves, and every colour variant with the other members of its group
(Koopas, Paratroopas, Toads, Shy Guys, Piantas, Nokis, Bros, Magikoopas, Dry
Bones). Written per frame, like the 00-type byte writes it replaced, in every
rel state because the table is read during the match too. Nothing to undo when
off: the stock values come back with the next table load from disc.

## Dup-load model bind (force-swap boots)

When a roster holds the same character twice, the model-bind loop at
`0x800156B0`/`0x800156E4` finds a NULL table entry for the second copy. Two
C2 hooks substitute a scratch object at `0x80370F1C`:

- `0x800156B0` (`DupLoadBindCharID`): r5 = `&table[r9]`; r3 = `table[r9]`, or
  the scratch when NULL.
- `0x800156E4` (`DupLoadBindStore`): `*(r3 ? r3 : scratch) + 24 = model[r8]`,
  where `model[r8] = *(0x8036E548 + r8*4 + 11456)`.

Both replace the instruction they overwrite (no `.instruction`). The second
needs two GPRs, so it reads the backup frame directly: saved r<n> is at
`r30 + 0x8 + (n-3)*4` (`READ_GAME_REG` declares its own r30 alias and cannot
be used twice in one function).
