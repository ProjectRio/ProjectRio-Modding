# The MSSB text engine, and the ScreenText / ScreenList helpers

Used by: `Include/Rio/ScreenText.h`, `Include/Rio/ScreenList.h`,
`RioModPack/Options Menu.c`.

## The engine

All in main.dol (`0x8000F988-0x80010FA0`), valid in every game state. Also
used by `Gecko Codes/Match/Write Text To Screen.c`, the original demo.

| address | what |
|---------|------|
| `0x80366B18` | `screenTextArray`: the pool of 30 `ScreenText` blocks (56 bytes / 14 words each, 4-aligned in RAM despite the packed decl) |
| `0x80035674` | the scene draw pass: calls `DrawTextOnCondition(group)` for groups 1..8 every frame, rendering every block whose state (+0x2A) is 2 and whose group (+0x2B) matches |
| `0x80010F2C` | `DrawTextOnCondition(1)`: the text call inside `maybeProcessUIUpdates` (`0x80035168`), AFTER the UI element loop |
| `0x8000FF04` | `initializeTextParameters`: the game's allocator; scans blocks 0..29 and takes the first free one, and can only point a block at strings from the game's text banks |
| `0x8000FE54` | frees all 30 blocks (scene transitions) |
| `0x80366218` | the glyph remap table (`0x8000 \| n` in a string skips it: raw glyph index) |
| `0x800E98E0` | inline control-code dispatch table (see below) |
| `0x800E8700` | `g_d_GameSettings.FrameCountWhileNotAtMainMenu` (u16), the default frame stamp |

To put text on screen you only fill in a block and keep it active; the engine
draws it every frame, in every scene. The helper skips the allocator: it
claims the TOP blocks of the pool (`TEXT_FIRST_BLOCK = 30 - TEXT_SLOTS`, which
the allocator only reaches with 30 texts live at once) and fills the fields
itself, pointing them at its own buffers. Blocks are re-claimed every frame,
which self-heals after scene transitions. The string must live somewhere
permanent: the engine keeps the pointer and reads it every frame. The helper's
buffers (and any static data in a gecko code) qualify, since CGecko embeds
them in the C2 payload, which stays resident for the session.

### Block fields by offset

| offset | field |
|--------|-------|
| +0x04 | pointer to the glyph string |
| +0x08 | colour (RGBA) |
| +0x10 / +0x12 | x, y |
| +0x18-0x1E | four u16s inserted by control codes 0x400F-0x4012 |
| +0x28 | `currLetterBeingDrawn`: max letters to draw, used by dialogs for the typewriter effect (a button press writes 10000 to skip to the end); -1 = show all |
| +0x2A | state: 2 = active, 0 = free |
| +0x2B | draw group |
| +0x2C | font style: 0 = large 22px, 1/2 = small 18px |
| +0x2F | justify: 0 left, 1 centred on x, 2 right |
| +0x30 | vertical alignment mode; 0 = y is the top line |

### Block fields (as filled by `WriteTextV`)

| field | value |
|-------|-------|
| `bankText` | `u16*` glyph buffer, terminated by `0x4000` |
| `color` | `0xRRGGBBAA`; `DrawText` fades RGB but preserves the alpha byte, so AA < FF gives translucent text |
| `x`, `y` | screen is 640x448, centre (320,224) |
| `maxLettersToDraw` | -1 = show all |
| `drawGroup` | 5 (groups 1-8 are drawn every frame) |
| `style` | 0 = large (22px), 1 = small (18px); the font has exactly two sizes |
| `lineSpacing` | 2 |
| `justify` | 0 left, 1 centre, 2 right |
| `state` | 2 = active; set LAST. 0 = free |

The menu uses the SAME engine as everything else: there is no separate menu
text function, the game just points a block at a pre-wrapped small string
(character bios on Records, item-shop popups).

### Glyph encoding

ASCII -> glyph index in the font's order: `'!'..'['` = 0.., so `'-'` = 12,
`'.'` = 13, `'0'..'9'` = 15..24, `'?'` = 30, `'A'..` = 32.., `']'` = 59,
`'^'` = 60, `'a'..'z'` = 62.., `'{'` = 88, `'}'` = 89, `'~'` = 90. Space and
newline are control codes `0x4002` / `0x4001`, end of string `0x4000`, wide
space `0x4003`, `0x400F-0x4012` insert the number held in the block's
+0x18-0x1E slots. (`MSSB Text to Hex/MSSB Text to Hex.py` does the same
mapping offline.) The font
has A-Z a-z 0-9 and `!"#$%&'()*+,-./:;<=>?@[]^{}~`; there is NO backslash, `_`,
backtick or `|`, and no tab: unsupported characters render as `?`. The font is
proportional (about 12 px per glyph at small size) and its space glyph is
narrower than a digit, so only the tabular digits align; zero-padded numeric
fields are the way to line columns up.

### Inline colour opcodes

Control code `0x4000 | opcode`. The palette was read live from Dolphin RAM
while idling on the Challenge Mode item shop (2026-07-29). The engine ORs the
table's RGB with the block's own alpha byte.

| opcode | tag | RGB |
|--------|-----|-----|
| 1 | newline | - |
| 2 | space | - |
| 5 | `{reset}` | restores the block's BASE colour (no RGB of its own) |
| 6 | `{pink}` | `0xFF15B1` |
| 7 | `{gold}` | `0xB89000` |
| 8 | `{red}` | `0xFF0000` (item shop "easier" / "to get hits") |
| 9 | `{green}` | `0x009900` |
| 10 | `{blue}` | `0x0000FF` (item shop "(Only good next game.)") |
| 11 | `{maroon}` | `0x800000` |
| 12 | `{teal}` | `0x339966` |
| 13 | `{purple}` | `0x333399` |
| 14 | `{black}` | `0x000000` |

These are the ONLY colours markup can switch to mid-string. Always close a span
with `{reset}`; unknown `{...}` is emitted literally. Example reproducing the
in-game item shop popup verbatim (live-verified):

```c
WriteTextEx(70, 35, 0x323232FF, TEXT_SMALL, TEXT_LEFT,
    "A mysterious power makes it\n"
    "{red}easier{reset} for everyone {red}to get hits{reset}.\n"
    "{blue}(Only good next game.){reset}");
```

## ScreenText.h usage

```c
#include "Include/Rio/ScreenText.h"

ScreenTextTick();   // once, at the top of the hook
WriteText(320, 36, "Hello World!");             // white, large, centred
WriteText(320, 60, "Inning %d", g_Scores.Inning);
WriteTextEx(16, 400, TEXT_YELLOW, TEXT_SMALL, TEXT_LEFT, "P%d is batting", port + 1);
WriteMenuText(60, 150, 30, "The younger Mario bro. ...");   // small, left, wrapped at 30 glyphs
WriteTextWrapped(x, y, color, style, justify, maxChars, fmt, ...);
```

- A text lives for one frame; call it every frame you want it shown. Stop
  calling and it disappears, PROVIDED `ScreenTextTick()` runs every frame: it
  frees the previous frame's blocks, so a frame that draws nothing still
  releases them. `WriteText` also ticks internally.
- The best home is a per-frame code (a `CGECKO()` with no `.address`): a hook
  inside a HUD draw function stops running whenever that element hides.
- Format codes: `%d %u %x %f %s %%`, `\n` starts a new line. `%f` prints two
  decimals with trailing zeros kept. Minimum width `%6f` -> `"  5.60"`, zero
  padded `%06f` -> `"005.60"` (`-2.25` -> `"-02.25"`, the sign stays first).
- Limits: `TEXT_SLOTS` texts per frame (default 8), `TEXT_MAXLEN` glyphs each
  (default 47); overflow is silently dropped. Word wrap works in glyph space; a
  single word longer than `maxChars` overflows rather than being split.
- If two SOURCE FILES both use the header, give each its own block range with
  `TEXT_SLOTS` / `TEXT_FIRST_BLOCK` before including it: each compiled file
  has a private copy of the state, so the ranges must not overlap.
- `SCREENTEXT_NO_FLOAT` drops the `%f` path (and the FPU use) from a build.
- A full character bio needs a bigger `TEXT_MAXLEN` than the default 47: each
  `{tagname}` span costs a glyph on top of the literal text. With no
  `TEXT_BUFFER_ADDR` the only cost is payload size, not RAM claims.
- Where to hook a text demo: a per-frame code (no `.address`,
  `.state = MSSB_GAME`). The Write Text demo was once injected inside
  `draw_ongoingStarGuageHud`, but that is the BATTING star gauge HUD: the game
  stops drawing it the moment the ball is in play, so the hook never ran during
  the liveball scene.

### Where the state lives

By default the state (frame stamp, slot counter, glyph buffers) is ONE
initialised struct in `.data`: CGecko embeds `.data` in the payload
(persistent for the session), while a zero-initialised variable would land in
`.bss`, which is NOT part of the payload and would point into whatever follows
the code list. The nonzero `lastFrame` initialiser forces `.data`. The cost is
payload size: `TEXT_SLOTS*(TEXT_MAXLEN+1)*2` bytes of mostly-zero gecko lines.

To shrink the payload, `#define TEXT_BUFFER_ADDR` (before the include) to a
claimed-free-memory address: the state moves there and vanishes from the
payload. Claim `8 + TEXT_SLOTS*(TEXT_MAXLEN+1)*2` bytes in
`ClaimedFreeMemory.h`. `lastFrame` self-initialises (whatever is there differs
from the live counter). Give each code its OWN address.

### The frame clock in menus

`ScreenText_FrameNow` defaults to `FrameCountWhileNotAtMainMenu` (u16 @
`0x800E8700`). That counter reads 0 AND NEVER MOVES anywhere in the menu REL
(live-verified 2026-08-28 with the Options scene on screen: the counter sat at
0 and `nextSlot` sat at `TEXT_SLOTS`, full). So `ScreenTextTick()`
early-returned every frame, every slot stayed claimed from the first frame,
and every later `WriteTextEx` was silently dropped: the screen was a FROZEN
SNAPSHOT of frame 1, which made the cursor look dead while the D-pad and A were
working the whole time.

A menu scene runs exactly once per frame (the dispatcher calls
`screenFuncTable[sc]()` once), so counting its own calls is an exact frame
clock: `#define ScreenText_FrameNow g_frame` before the include, with `g_frame`
at an ABSOLUTE claimed-RAM address (the Options menu uses `0x802EB528`), not a
payload static: cgecko reaches payload statics through r31, which read NULL
from inside a helper (`ScreenTextTick`) while prototyping Custom Menu Scene.c.

### Compiler flags in the header

The implementation is compiled `-Os` through `#pragma GCC optimize`. That
pragma resets other `-f` flags to their defaults, so the ones CGecko needs for
correctness are restated: `no-jump-tables` (jump tables embed unrelocated code
addresses), `no-optimize-sibling-calls` (a tail call out of the naked entry
would skip the payload's register-restore epilogue),
`no-tree-loop-distribute-patterns` (zeroing loops must not become `memset`
calls; there is no libc).

### Text slot budget (Options menu example)

The Options menu sizes `TEXT_SLOTS 22`, the worst case for one frame:
options page = 1 title + 1 footer + 1 cursor + 5 rows x 2 + 9 note lines = 22;
music page = 3 + 9 rows x 2 = 21. Label and value are separate blocks at fixed
x because the font is proportional; padding one string with spaces would leave
the ON/OFF column ragged. `TEXT_MAXLEN 27` is the longest string drawn (the
notes panel is 24 glyphs wide); the buffer is `8 + 22*28*2 = 1240` bytes at
`0x802EB050`. Two things cap this: claimed RAM (the pack's UI and Custom Music
share one verified-free run `0x802EAF90-0x802EB83F`, 2032 bytes usable from
`0x802EB050`: 1240 buffer + 4 g_frame + 4 g_page + 16 s_music + 64 music config
+ 4 magic + 120 saved descriptors + 480 path buffers = 1932) and the game's 30
blocks (22 leaves the game only 8). Notes are wrapped one block PER LINE rather
than through `WriteTextWrapped`, which packs a paragraph into ONE block and
would force `TEXT_MAXLEN` to ~160 (a 5482-byte buffer).

## ScreenList.h

A self-contained "menu list" widget: N items, a movable selection index, and a
scrolling window when the list is longer than what fits on screen, the same
shape as the Challenge Mode item shop's list.

```c
#include "Include/Rio/ScreenList.h"

static ScreenList s_shopList;
ScreenList_Init(&s_shopList, 3, 5);              // once: 3 items, 5 rows visible

// every frame, after your own edge-detected button reads:
const char* items[] = { "Weight Bat", "Iron Glove", "Fast Cleats" };
if (pressedDown) ScreenList_MoveDown(&s_shopList);
if (pressedUp)   ScreenList_MoveUp(&s_shopList);
ScreenTextTick();
ScreenList_Draw(&s_shopList, 60, 100, 24, TEXT_SMALL, items);
// s_shopList.selected is the chosen index
```

- Each visible row costs ONE ScreenText slot for its label plus ONE MORE for
  the selected row's cursor glyph: size `TEXT_SLOTS` to at least
  `visibleRows + 1`.
- Selection wraps at both ends; the window auto-scrolls to keep the selection
  on screen and only slides as far as needed (no re-centering).
- The header reads no input; pass in your own edge-detected up/down so it is
  usable from game-state or menu-state code alike.
- `LIST_CURSOR_WIDTH` (default 20 px) reserves room for the cursor so labels
  start at the same x whether or not their row is selected.
- Labels are a plain array rather than a callback because it is less code at
  the call site.

**Scope note.** The shop's own item list turned out to be drawn from
PRE-RENDERED TEXTURES (baked text + a hand-icon sprite), not the dynamic text
engine, so there is no game function or struct to reuse for it, and reversing
the exact texture objects live would need slower RAM work (or a debugger) than
a couple of snapshots can isolate. The widget is a from-scratch replacement on
top of ScreenText.h: it draws labels with the dynamic engine and a coloured `>`
as the cursor. It will not look pixel-identical to the shop. If you want the
hand-cursor/list textures as image assets, Dolphin's texture dumper (Graphics >
Advanced > Dump Textures) is far more direct than reversing GX TextureObj
structs out of RAM.

**Historical note.** An array of string literals used to CRASH any hook that
drew a list, because cgecko did not relocate pointers the linker baked into the
payload's data section (it only rewrote addresses FORMED by code). That was a
cgecko bug, fixed 2026-07-31: cgecko now emits a runtime fixup loop for such
pointers (`find_picdata_relocs` / `build_pic_reloc_fixup` in cgecko.py).
Ordinary C data tables are safe; keep cgecko current if an old checkout shows
odd data-pointer crashes.
