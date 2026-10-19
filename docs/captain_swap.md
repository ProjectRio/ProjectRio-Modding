# Captain Swap

Used by: `Gecko Codes/Rio Built-in/Captain Swap.c` (hook at `0x8064F67C`,
menus.rel, `MSSB_MENU`).

## Hook site

`characterSelectControls` (decomp `src/menus/text_0323C.c`, menus.rel .text
`0x104FC`) reads the acting team's pad state and does

```c
if ((newInput & 0x1000) != 0) goto TAIL;   // Start pressed
```

The `bne` of that test is the instruction at `0x8064F67C`. The code replaces
it and never re-issues it: when Start is not held nothing happens (the branch
would not have been taken), and when it is held the swap runs in place of the
game's own Start handling at the branch target, so Start on the team-select
screen swaps captains instead of doing what the stock game did. At the site
r30 holds `newInput`, r27 the acting team (0/1), and r0/cr0 are rewritten by
the very next instruction, so a C body is safe.

menus.rel loads at `0x8063EFC0` (module .text at `0x8063F094`), the same slot
as game.rel; this was confirmed by matching the overwritten instruction of
every menus.rel hook in this folder.

## menus.rel state the code edits

Not yet typed in the decomp (`lbl_2_bss_F468` / `menuCursors`):

| address                | meaning                                              |
| ---------------------- | ---------------------------------------------------- |
| `0x80750C48 + 4*team`  | s32 cursor position on the character grid (0..8 = a roster slot) |
| `0x80750C89 + team`    | byte, cursor is on the bottom half of the screen     |
| `0x80750C8D + team`    | byte, a player profile is being shown                |
| `0x803530EC`           | word passed as the first argument of `teamSelectionSetChemStars` (Static_Stats_Tables + 0x474C) |

The captain background is a texture override on UI records 176 (team 0) and
177 (team 1) of `menuGraphicsStructures` (`0x8039C3E0`, 0xC0-byte records):
`textureOverride[1]` at record offset 0xAE, so `0x803A488E + 0xC0*team`.

Everything else is typed: `cursorPositions.roster` (`rosterCharID`,
`positionSwapMapping`, `chemWCaptain`), `Static_Stats_Tables.captainSelectedID`,
`Static_Stats_Tables.characterStats[cap].chemistry`, `captainIDMappings`
(the 12 allowed captains at `0x80108ED0`).

## Sounds

`sndFXStartEx(0x1BC | 0x1BA, vol, 0x3F, 0)`. The original asm passed the whole
word at `lbl_800EFBA4` as the volume; the engine forwards it untruncated, and
the decomp's own captain-select calls pass a single byte of that table. The C
version passes byte 3 (the low byte of that word).
