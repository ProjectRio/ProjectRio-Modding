# Building and editing menu scenes in C

How MSSB draws its 2D menus, and how a mod can add to them, change them, or put
up a screen of its own using the game's own machinery. Everything below was
traced live on the US disc and is what `RioModPack/Online Menu.c` runs on;
`Include/Rio/MenuScene.h` wraps the pieces named here.

## The model

A menu screen is three things:

1. **A container** loaded from ZZZZ.dat under a small integer **tag**. It holds
   textures and a **layout**: a table of *elements*, each a list of *parts*
   (quads with vertex colours and/or a texture, anchors that place child
   elements, colour-set variants). Loaded containers sit in 20 *texture slots*
   (`0x803C4BE0`, `0x3C` bytes each); the tag of each slot is in the s16 table at
   `0x8023D6C0`. Tag 1 ("menu bars and cursors", entry 1745) stays loaded
   through every menu; the main menu adds tags 6, 9, 15 and 18.
2. **UI records** in one DOL pool: 864 records of `0xC0` bytes at
   `menuGraphicsStructures` (`0x8039C3E0`). A record names a slot and an element
   in it, has a visible flag, an animation frame, a position, a colour
   multiplier, an optional parent, and ten per-part texture overrides. The draw
   pass (`maybeProcessUIUpdates`, `0x80035168`) walks the pool by index each
   frame and draws every in-use, visible record, then the text pass.
3. **A scene node**: the draw-script item a screen registered. The game builds
   the screen's records from a **descriptor list** with
   `addGraphicsElementToScene(node, list)`, and every record is then reachable
   by **handle** (its position in the list) through
   `graphicsRelatedArray[node->base + handle]`. `removeGraphicsElementFromScene
   (node)` frees them all by handle.

Parents matter. A child record is drawn only on frames its parent *attached*
it: when the parent draws, `fn_80034BCC` walks the parent's child list and, for
each child whose element and *sub-index* (`+0x72`) match one of the parent's
layout anchors, copies that anchor's matrix into the child (`+0x78`) and sets
`+0xA8`. That is how the main menu's six unselected highlight bars stay
invisible with their visible flag set, and it is why a copied record placed
under the same parent lands exactly where the original does.

The full record layout, descriptor format, layout structures and routine
addresses follow; `Include/Rio/MenuScene.h` wraps them.

### The UI record pool

864 records of `0xC0` bytes at `menuGraphicsStructures` (`0x8039C3E0`). A
record with flags (+0x54) == 0 is free. Fields, as read by
`maybeProcessUIUpdates` (`0x80035168`) and `allocateGraphicsSlot`
(`0x80034F50`):

| offset | type | field |
|--------|------|-------|
| +0x00 | u32 | parent record, 0 = top level |
| +0x04 | u32 | next sibling in the parent's child list |
| +0x08 | u32 | first child |
| +0x48 | f32 | position x, offset in the parent's space |
| +0x4C | f32 | position y |
| +0x50 | f32 | position z |
| +0x54 | u32 | flags: 0 = free, bit 1 (value 2) = visible, bit 2 = text record |
| +0x58 | u32 | RGBA multiplier on the layout's vertex colours |
| +0x5C | u32 | animation frame, 16.16 fixed |
| +0x60 | u32 | frames per frame, 16.16 (normally 0x10000) |
| +0x64 | u16 | layout element index in the container |
| +0x66 | u8  | texture slot (the loaded container) |
| +0x67 | u8  | draw layer (from the descriptor) |
| +0x68 | u8  | play: 0 stopped, 1 count up, 4 count down |
| +0x72 | s16 | sub-index: which of the parent's anchors places this child |
| +0x78 | 3x4 | matrix the parent's anchor wrote this frame |
| +0xA8 | u8  | attached: set when the parent attached it this frame, cleared after drawing |
| +0xAC | u16[10] | per-part texture override, 0xFFFF = none |

Positions are written as IEEE bit patterns (`MS_F32_*`) so hooks never touch
the FPU.

### Scene nodes and handles

`graphicsRelatedArray` at `0x80371C30`, 8 bytes per entry: +0 the record, +4
the owning node (first entry only). A node's handles start at the u16 at
`node+0x14` and number `node+0x16`. `currentDrawingItem` (`0x803CC1B8`) is the
node whose function is running. `MS_Record(node, h)` reads
`*(u32*)(0x80371C30 + (base + h) * 8)`.

### Descriptors

One 0x20-byte record descriptor, as `allocateGraphicsSlot` reads it: element
index (u16 at +2), mode at +0x10 (0 = visible and stopped, 1 = visible and
playing, 2/3 = created hidden), layer at +0x11, parent handle (u16 at +0x12,
0xFF = top level), container tag at +0x14, sub-index (u16 at +0x1E). A list
ends with a descriptor whose type byte (+1) is 3.

### DOL routines

| address | routine |
|---------|---------|
| `0x800054F4` | `memcpy` |
| `0x8006E894` | `DCFlushRange` |
| `0x80034E20` | `addGraphicsElementToScene(node, descriptors)` |
| `0x80034CEC` | `removeGraphicsElementFromScene(node)`: frees by handle, never walks children |
| `0x80034F50` | `allocateGraphicsSlot` |
| `0x80034BCC` | the parent -> child anchor attach |
| `0x800363D8` | `load_Icon(node, handle, part, table, idx)`: set a part texture from the container's icon table |
| `0x800625A4` | `updateCharacterSelectProcessCode(channel, code)`: post a menu process code |
| `0x800626EC` / `0x80062674` | `makeCursorUnmovable` / `makeCursorMovable` (counted; the latter also zeroes the process code) |
| `0x80640234` | `changeScreenVariables(screenCode)` (menus.rel) |

### Containers: texture slots, texture records, layouts

A loaded container occupies a texture SLOT (`0x803C4BE0`, 20 slots of `0x3C`
bytes): +0x30 the buffer, +0x34 its texture header, +0x38 its layout. The s16
tag table at `0x8023D6C0` (-1 = free) maps tags to slots.

Texture header: u16 count, then 0x20-byte records (pointers already relocated):

| offset | field |
|--------|-------|
| +0x00 | u16 index |
| +0x04 | pixel data pointer |
| +0x08 | palette pointer |
| +0x0C | u16 height |
| +0x0E | u16 width |
| +0x1B | u8 GX format (8 = C4, 9 = C8, ...) |
| +0x1C | u16 palette entries |
| +0x1E | u8 palette format |

Layout: +8 -> element table `{u32 count|flags; u32; u32 ptr[count]}`;
element -> `{u32 nparts|flags; u32; u32 part[nparts]}`; part -> `{u16 count,
u16 size, sub-records...}`. Sub-record contents are element specific; anchors
keep x at +0x14 and y at +0x16 of the first sub-record (i.e. part+0x18 /
part+0x1A), and quad colours are RGBA words.

### The UI draw pass and the menu control block

`maybeProcessUIUpdates` draws pool records `[*(0x803CBC98), *(0x803CB814))`
each frame, then the text pass. The end bound is clamped to 0x360 by the game's
own code. Narrowing the pair hides everything else without touching it; the
Options menu blanks by setting the end to 0. Always restore.

`0x803CBBCC` -> `{u16 rel, u16 screenCode, u16 menuProcess, u16 prevScreen,
u16 prevProcess}`; `changeScreenVariables` sets prev = current, current = new,
process = 0. Main menu = 5, Options = 6, Dictionary = 7, Records = 8.

### Rules of the road for cgecko payloads

- No mutable payload statics reached from helpers: cgecko reaches payload data
  through r31, which is NULL inside a helper. Keep mutable state at claimed RAM
  addresses and form pointers to read-only tables in the hook body, passing
  them down as arguments. Read-only tables (`.picdata`, like string literals)
  are fine.
- No floats in hooks unless the hook saves the FPU.
- Pixel data the GPU reads in place must be 32-byte aligned and live in the
  mod's image, never inside a game texture's area (a "spare" one turned out to
  be Toy Field's preview picture).

## Tracing a stock screen

The method that produced all of the above, in the order it pays off:

- **Dump the pool** on the screen you care about: every in-use record with its
  parent, element, slot, sub-index, frame, play mode and overrides. The
  scratch tooling reads Dolphin's RAM through the `dolphin-emu.<pid>` mapping
  (see the memory notes on the live harness). Records created together are
  consecutive and their order is the descriptor order, so handles fall out of
  the dump.
- **Diff the pool across one input** (a D-pad press) to see which records the
  screen's controller flips: visibility, frames, overrides.
- **Hide records one at a time** (clear bit 1 of `+0x54`) and screenshot. This
  is the fastest way to learn what each element draws; it found the gradient,
  the top bar, the watermark and the hint text of the Options screen in two
  passes.
- **Read the layout** of the container behind a record: the element's parts
  show its vertex colours, texture indices and anchors. Anchor y values are
  the row positions of a list; colour words are what a screen is tinted with.
- **Find the controller**: the menus.rel function set as the node's function
  (main menu: `fn_2_747FC`). Its constants give the animation start and end
  frames per direction, and which handles it touches. If a mod changes the
  cursor range, every handle computed from the cursor has to be accounted
  for.

Use the built folder under Dolphin for testing, never a savestate: the pool
and slots are runtime state and the menus REL is not resident in game
savestates.

## Recipes

Each of these is live in `Online Menu.c`; the header functions are named.

### Copy a stock record one row over

To add a button that the game draws and animates like its neighbours, copy
the neighbour's records and hang the copies in the same tree:

```c
u32 column = MS_Record(node, 2);            /* the parent that owns the rows */
u32 bar    = MS_CopyRecord(MS_Record(node, 3), column);
MS_POSY(bar) = MS_F32_NEG(37);              /* one row up, in the parent's space */
```

The copy keeps the original's element and sub-index, so the parent's anchor
places it where the original is; the record's own position is applied on top.
Give the copy a different texture through its part override
(`MS_OVERRIDE(rec, 1) = index`) when the element's art is a texture, as the
main-menu labels are. Copy children after parents (the highlight bar's shine
is a child of the bar) and free them in the opposite order with
`MS_DropRecord`, which unlinks from a still-live parent. Do it on the frame the
game tears its scene down (hook the controller: on the main menu the trigger is
`Static_Stats_Tables+0x472A == 0` at the node function's entry), because
`removeGraphicsElementFromScene` frees by handle and knows nothing about the
copies.

### Show your own art

A record draws whatever texture its part names, by index into the container.
Point an unused texture record at pixels shipped in the mod:

```c
static const u8 s_pixels[1920 + 32] __attribute__((aligned(32))) = { ... };
u32 hdr = MS_TextureHeaderOfTag(6);
MS_RetargetTexture(hdr, 50, 49, s_pixels, s_pixels + 1920);   /* t50 now looks like t49's kind of image, with our pixels */
```

Encode with `RioModPack/MakeOnlineTexture.py` (C4 with an IA8 palette for the
labels, C8 with an RGB5A3 palette for the preview pictures; both formats the
stock art uses). The container is re-read from disc on every non-resume entry
to the screen, so redo the retarget when the scene is built. Keep the pixels in
the mod's image, 32-byte aligned; never park them in another texture's area.

### Edit the layout in place

The loaded layout is plain RAM. The main menu's seven row anchors were moved
from a 43 px pitch to 37 px to fit an eighth row inside the same box:

```c
u32 elem = MS_LAYOUT_ELEM(layout, 56);        /* the column element */
for (i = 0; i < 7; i++)
    MS_PART_ANCHOR_Y(MS_ELEM_PART(elem, i + 1)) = 137 + 37 * i;
```

Check the stock values before writing so an unexpected layout is left alone.
Colours the same way: `MS_RecolourElement(layout, 299, purple, red, 6)` turns
the Options gradient red, and calling it with the arrays swapped puts it back.
Layouts of always-resident containers (tag 1) are shared by every screen, so
reverse an edit on exit.

### Build records of your own

For a screen of your own, describe the records and build them on a node in
claimed RAM:

```c
static const u8 s_descs[] = {
    MS_DESC(301, 0, 22, MS_NO_PARENT, 1, 0),  /* handle 0: gradient container */
    MS_DESC(299, 0, 22, 0,            1, 0),  /* handle 1: the gradient, child of 0 */
    MS_DESC(236, 0,  4, MS_NO_PARENT, 1, 0),  /* handle 2: top bar container */
    MS_DESC(232, 0,  4, 2,            1, 0),  /* handle 3: the bar strip */
    MS_DESC_END
};
MS_BuildScene(node, s_descs);
MS_Hold(MS_Record(node, 0), 0x28);            /* park on its fully-shown frame */
```

`MS_RemoveScene(node)` frees them. The stock descriptor lists are worth
copying from: the DOL's are found by scanning for calls to
`addGraphicsElementToScene` (the standard menu frame is at `0x800FEB30`), the
REL's the same way in the live REL.

### Put up a screen and keep the rest off it

A custom screen replaces one of the scene table's functions (`Options Menu.c`
and `Online Menu.c` both use a C2 hook ending in `blr`, on the Options handler
and on the unused assert stub that screens 2 and 3 point at). Whatever the
previous screen left in the pool is still drawn, so either blank the draw pass
(`MS_DRAW_END = 0`, the Options Menu's way; its text still draws) or narrow it
to your own records:

```c
MS_SceneIndexRange(node, &lo, &hi);
MS_NarrowDraw(lo, hi, &savedStart, &savedEnd);
...
MS_RestoreDraw(savedStart, savedEnd);
```

Restore on every way out. A per-frame watchdog that restores when the screen
is no longer current is the pattern both mods use.

Leave a custom screen for the main menu with `MS_ReturnToMainMenu()`: it
writes the control block as an Options exit would, because `mainMenuScreen`
only resumes without a reload when the previous screen was 6, 9 or 12.

### Animate like the game does

Records animate by frame counter: `MS_Play(rec, start, MS_PLAY_FORWARD)`,
then each frame `MS_StopAt(rec, end)` until it reports done. Timelines are per
element and have segments; the main-menu bar's are documented in
`Online Menu.c` (appear, arrive from above, arrive from below, wrap). Lock
input across a move with `MS_makeCursorUnmovable(0)` and release it with
`MS_makeCursorMovable(0)`. The game's own controller drives the same
timelines by posting *process codes* (`MS_updateProcessCode(0, 0x56)` for
"cursor moved" on the main menu); do not post one for a state the controller
does not know about, since its handlers index handles from the cursor.

## Gotchas collected on the way

- `+0x72` is signed and copied from the descriptor; the main menu numbers its
  rows 6 down to 0.
- A record with a parent but no matching anchor never draws, and its `+0x78`
  matrix is stale: place copies under the parent that has the anchor they
  expect.
- Text on a screen is not in these records: it is a text channel drawn by the
  text pass after the element loop (`Include/Rio/ScreenText.h` drives that
  engine).
- The preview-picture timeline fades in over frames 0..10 and out over 10..20
  and then holds; freezing it needs a "reached or passed" check, which is what
  `MS_StopAt` does.
- `makeCursorMovable` also zeroes the process code; harmless, the controller
  ignores code 0.
- Everything in the pool survives a screen change unless something frees it.
  The Options round trip leaks one draw node per trip even in the stock game;
  do not add to that.

## The Online button (`RioModPack/Online Menu.c`)

Adds an eighth button, "Online", ABOVE Exhibition Game on the main menu, at
main-menu index -1: Up from Exhibition Game lands on it, Down from Options
wraps round to it, and it wraps the other way too. Selecting it opens a
placeholder screen (the Options backdrop recoloured red) that B backs out of.
The button is drawn by the game's own widgets. Traced live 2026-09-10, US disc,
menus.rel.

### How the main menu is built

The buttons are not text: each label is a 156x21 4-bit texture in the container
the menu loads under tag 6 (ZZZZ.dat entry 1740, textures t49 and t52-t57).
`mainMenuRelated`'s state 0 builds the screen from a descriptor list; the
controller (menus.rel `fn_2_747FC`, the per-frame node function) addresses
records only by handle, in descriptor order:

| handle | record | layout element |
|--------|--------|----------------|
| 0..2 | background, panel frame, the button column (parent of the rest) | column = 56 |
| 3..9 | the orange highlight bar, one per button | 57 |
| 10..16 | the grey idle bar, one per button | 59 |
| 17..23 | the highlight bar's animated shine (a CHILD of the matching 57 record) | 55 |
| 24..30 | the label, one per button | 53 |
| 31 | column art | 49 |
| 32, 33 | the preview picture and its crossfade partner | 48 |

A button's position comes from the column: its layout has one anchor per
(child element, sub-index) pair, and the DOL matches each child by element +
the s16 at +0x72 (6 for Exhibition Game down to 0 for Options), copying the
anchor's matrix into child+0x78 every frame. The record's own position (+0x48)
is applied on top. So a copy of an Exhibition Game record with pos.y = -37 (one
squeezed row pitch) draws one row ABOVE it with every property of the original.

Records with a parent are drawn only on frames the parent attached them
(+0xA8). The selected bar is the one whose visible flag (+0x54 bit 1) is set;
the controller flips those flags, kicks the shine (+0x5C frame, +0x68 play) and
the label pulse, and crossfades the preview picture in `fn_2_73EFC`/`fn_2_73CBC`
while process code 0x56 ("cursor moved") is pending. With a cursor of -1 those
helpers would index handle 2 (the column) and hide the whole menu, so any
transition touching -1 is done by the mod and the stock helpers are never told.

### Game objects

| address | what |
|---------|------|
| `0x80750BF0` | `lbl_2_bss_F410[0]`: the main-menu cursor (NOT `0x8074C023`) |
| `0x80750BF4` | `[1]`: where it was before the last move |
| `0x8074C020` | the controller's "cursor as drawn" copy |
| `0x803530A8` | `Static_Stats_Tables.mainMenuOptionSelectedIndex` (u8) |
| `0x803530CA` | `Static_Stats_Tables+0x472A`: 0 = tear the button scene down this frame |
| `0x806B3890` | `fn_2_747FC` prologue (`stwu r1,-0x20(r1)`): the per-frame node function |
| `0x80641494` | D-pad branch: the `bl updateCharacterSelectProcessCode(0,0x56)` after the cursor step/wrap |
| `0x80641294` | A branch, just before `switch (cursor)` (`lis r4, 0x8075`) |
| `0x806413A4` | A branch, the `bl updateCharacterSelectProcessCode(0, 0x58)` "button pressed" animation |
| `0x8064179C` | state 8, the Options transition's `bl changeScreenVariables` (`li r3,6` is the instruction before) |
| `0x80641798` | that `li r3, 6` |
| `0x806402B0` | the assert stub `screenFuncTable[2]`/`[3]` point at: becomes the Online scene |

### Textures

Nothing on the main menu uses t50 or t51 (two 10x72 strips). After the
container loads, record 50 is rewritten to describe the 156x21 C4 label and
record 51 the 357x302 C8 preview picture (copies of t49's and t63's records
with our pixel/palette pointers), from `RioModPack/OnlineTexture.h` (generated
from `assets/Online.png` and `assets/OnlinePanel.png` by `MakeOnlineTexture.py`;
32-byte aligned). The container has 122 textures, which is the sanity check.
The container is re-read from disc on every non-resume main-menu entry and the
patch is redone each time the scene is built, so other screens using entry 1740
see a stock copy. The icon table for the preview pictures is 0x32.

### Row spacing

Stock rows sit 43 px apart from the column's anchors at y = 100..358 inside a
box drawn to that height. The seven anchors (column element 56, parts 1..7,
first sub-record) are rewritten to 137..359 (37 px apart) and the Online row
takes y = 100. Every value is checked against the stock or already-squeezed
layout before any is written.

### Lifecycle

- **Build**: hook `fn_2_747FC`'s prologue. It runs once per frame from the
  frame after state 0 built the scene until the teardown frame
  (`+0x472A == 0` on entry). First sight of a scene -> build our four copies
  (bar, grey bar, shine, label; creation order is draw order); teardown frame
  -> drop them before the game frees the parent. Pool full -> no button rather
  than half of one.
- **Default**: the cursor starts on Online. It is set to -1 before the
  controller's entry animation (which then shows no stock highlight, and its
  "wait for the bar" phase counts zero bars, so it completes on the picture's
  fade-in alone).
- **Cursor move** (`0x80641494`, hooked instruction nopped): both wraps become
  -1 (Up from 0 -> 6 becomes -1; Down from 6 -> 0 becomes -1). From -1 the
  stock arithmetic already does the right thing (-1-1 < 0 wraps to 6; -1+1 = 0).
  A move touching -1 is drawn by `MoveHighlight`; every other move posts 0x56
  as before.
- **Select** (`0x80641294`): the switch is a jump table guarded by an UNSIGNED
  compare against 6, so -1 lands on the common tail "menuProcess = 5, confirm
  whatever mainMenuOptionSelectedIndex holds". The hook sets that to Options (6)
  and arms a latch; `0x806413A4` skips the pressed animation for -1 (its handler
  indexes handle 17 + cursor, i.e. Options' grey bar). The stock Options
  transition runs unchanged (fade, teardown, state 8) up to its
  `changeScreenVariables(6)` at `0x8064179C`, which the latch redirects to
  `ONLINE_SCREEN_CODE`.
- **Exit**: `MS_ReturnToMainMenu()` writes the control block as an Options exit
  would (current 5, prevScreen 6, state 0), so `mainMenuScreen` resumes
  (prevScreen 6 -> state 12) rather than cold-reloading against containers that
  are still loaded.
- **Watchdogs** (`MSSB_ALWAYS`): free the copies if the scene went away by a
  route the node hook never saw (REL swap, savestate, crash path), detected by
  the source record being free or the control block gone; drop the backdrop if
  the Online screen is no longer current.

### Highlight animation (layout element 57)

Bar timeline, from the controller's constants:

| frames | segment |
|--------|---------|
| 0 .. 0xE | appear in place (entry, and arriving by wrap-down) |
| 0xF .. 0x1D | arrive from the row above (moving down) |
| 0x1E .. 0x2C | arrive from the row below (moving up) |
| 0x2D .. 0x36 | wrap segment: played forward by the bar leaving off the bottom, backward (0x36 -> 0x2D) by the bar arriving at the bottom from the top; the bar leaving off the top runs 8 -> 0 backward |

The shine ends at 0xE and the picture's fade-in at 0xA (fade-out 10..20, then
hold; freeze with a >= check). Play mode 1 counts up, 4 counts down. Phase 0
(`fn_2_73EFC`) starts the timelines, crossfades the picture (incoming on handle
0x20 from frame 0, outgoing on 0x21 from frame 10), pulses the label and locks
input; phase 1 (`fn_2_73CBC`) stops each timeline at its end frame and unlocks.
The mod reimplements both for moves touching -1. The entry slide-in
(`prev = ONL_ENTRY`) does not lock or set the picture up: the stock entry pass
does that itself.

### The placeholder screen's backdrop

Traced on the stock Options screen by hiding one record at a time: the purple
gradient is layout element 301 with element 299 as its child, and the top bar
is element 236 (a container) with children 232 (the bar strip), 237 (the
slanted title plate), 240 (title text, left out) and 238 (a separator line).
All live in the "menu bars and cursors" container under tag 1, so nothing is
read from disc. The purple is vertex colour in the layout parts (six shades in
299, one in 232's base colour set, two in 237); the same hue rotated to red is
written into the loaded layout on entry and put back on exit:

| purple | red |
|--------|-----|
| `0x643689FF` | `0x893636FF` |
| `0x431071FF` | `0x711010FF` |
| `0x141E3CFF` | `0x3C1414FF` |
| `0x906FDDFF` | `0xDD6F6FFF` |
| `0x7964C2FF` | `0xC26464FF` |
| `0x482267FF` | `0x672222FF` |
| `0x441458FF` (232) | `0x581414FF` |
| `0xA03ACBFF` (237) | `0xCB3A3AFF` |
| `0xDD8CFF80` (237) | `0xFF8C8C80` |

Rest frames: gradient container 0x28, bar container 0x14, bar art 0x7. The
records are built through `addGraphicsElementToScene` on a 24-byte node in
claimed RAM (`0x802EC378`) and the draw pass is narrowed to just them while the
screen is up.

Online Menu claimed RAM: `0x802EC32C` magic (`0x0A11E003`), then one word each
for valid, item, bar, grey, shine, label, armed, anim, animNew, animNewEnd,
animOld, animOldEnd, animShine, animCur, animLocked, bg, bgStart, bgEnd
(`0x802EC330`..`0x802EC374`), node at `0x802EC378`.

## The Options menu (`RioModPack/Options Menu.c`)

### REPLACE hook

MSSB menus are a per-frame jump table: `currentScreenFunctionChooser`
(`0x8064026C`) reads `menuCtrl->screenCode` and calls
`screenFuncTable[sc]()`, so a "scene" is one function. The mod hooks the
Options handler `screenFuncTable[6]` (`0x80658D98`, menus.rel) with
`.instruction = "blr"`: the re-executed instruction returns to the dispatcher's
`bctrl` the moment our body finishes, so the stock body (which begins
`stwu r1,-16(r1)`) never runs and there is no stack leak. Nothing reroutes: the
main menu's Options button already does `changeScreenVariables(6)` (the
`li r3,6` at `0x80641798`). `.state = MSSB_MENU` is REQUIRED: at rel == 5 that
address is game.rel.

**Why replace and not augment.** The first version was a C2 with
`.instruction = "stwu r1, -16(r1)"` drawing over the stock screen. Live testing
killed it: the cursor did not work (our list and the stock widgets were both
driven off `controllerInputs_`, and blanking the buttons after reading them was
not enough), and the stock art made the text unreadable. Replacing fixes both
at the source. The cost is the stock Options screen itself (audio/rumble
settings and the memory-card save its back-out confirm triggers) while the mod
is enabled.

`menuCtrl->menuProcess` is zeroed by `changeScreenVariables` on every change,
so `menuProcess == 0` is exactly "first frame of this entry" (the same
per-scene state var the stock handlers sub-dispatch on): the cursor resets to
the top each time without a separate latch.

`changeScreenVariables` is taken by raw address (`0x80640234`) rather than
through the menus symbol header: that header binds the file to MENU context
(`Include/Symbols/menus.h` sets `MSSB_CONTEXT_MENUS`, and game.h then #errors
"the game and menus RELs share one arena slot"), and the pack includes this
file alongside game-REL mods.

P1 input: `newInput` is already edge-detected by the menu's input gather (a bit
is set only on the frame the button is first pressed); reading it from an
external tool shows 0 because the gather clears it the same frame.

### The blank background

The Options button does `li r3,6; bl changeScreenVariables` and falls to its
epilogue with NO teardown, so the main menu is still fully drawn underneath.
(Writing screenCode=6 / menuProcess=0 by hand reproduces the transition
exactly, which is how this was tested without a controller.)

Two dead ends:
- `changeScene_(1,0)` (`0x800204CC`) only REQUESTS a scene by writing an id to
  `0x803716AD`; it never touches what is drawn.
- `resetAllDrawingStructs()` (`0x800B0BE8`) + `fn_80009144()`, debug.rel's own
  recipe, HANGS the menu: the dispatcher `0x8064026C` is itself a node in those
  banks, so wiping them stops `screenFuncTable[6]()` from ever being called
  again, and `fn_80009144` only re-adds the two DOL base draw functions.
  Silencing the menus.rel draw nodes individually changes nothing either; they
  only build state.

What the screen is made of (traced live 2026-08-28): the frame loop walks draw
nodes (`RunDrawScripts` `0x800B0CB8`). Node `0x80009094` (priority 0x7000)
draws the entire 2D screen; it is a two-line wrapper around
`maybeProcessUIUpdates` (`0x80035168`):

```
for (i = *(0x803CBC98); i < *(0x803CB814); i++)   // UI elements, 0xC0 B records from 0x8039C3E0
    draw element i;
DrawTextOnCondition(1);                            // 0x80010F2C: OUR TEXT
```

The element loop and the text call are separate and the text comes last, so
collapsing the loop's range (end bound `0x803CB814` = 0) draws no elements and
still draws our text. The code above the loop nudges the pair up and down under
a debug flag and clamps the end to 0x360, so this uses them, not corrupts them.
The saved bound lives at `0x802EB000` (`0xFFFFFFFF` = nothing saved) with a
magic at `0x802EB004` (`0x0B6D0FF`), inside the verified-zero run
`0x802EAF90-0x802EB83F`.

Not our bug, measured: entering screen 6 and returning leaks one draw node per
round trip (12 -> 16 over four) in the STOCK game too. At 64 node slots it
would take ~50 round trips to matter; it shows as a faint double-draw of the
main menu's first item after a few trips.

Other levers considered for a real background, cheapest first: draw each
string twice (black, offset a pixel) so it reads against anything, at 2 slots
per string; hook `insertGraphicDrawingFunction` (`0x800B0A5C`) while the stock
Options scene sets itself up and reject only its WIDGET callbacks, keeping its
background element (needs the Options element callbacks identified the way the
Dictionary scene's were); reset the drawing banks properly, re-registering the
menu dispatcher node with the DOL base functions.

### The watchdog

The scene blanks the framework and un-blanks it in the B handler. Leaving by
any other route (a crash, a savestate loaded with the screen open, another code
changing screens) would strand the end bound at 0 and leave EVERY menu blank
until reboot. `OptionsMenuRestore` runs `MSSB_ALWAYS` (the blanked variable is
main.dol state shared with the match) and restores the moment the current
screen is neither 6 nor the Online screen; it also ticks the text clock so the
text blocks are released (verified: forcing screenCode away left "MOD OPTIONS"
painted over the restored main menu). Its magic (`0x802EB004`) is initialised
in the watchdog, not the scene, because the watchdog runs from boot and claimed
RAM holds power-on garbage: without a one-shot the first frame could "restore"
garbage into the bound.

### Layout and the notes panel

Anchored top-left as a page that fills the 4:3 frame (640x448 XFB; nothing
reaches an edge because TVs overscan), because the content only grows. Column
x's were measured against the proportional font (~12 px/glyph small): label
"Duplicate Chars" from 52 ends near 235 so the value column starts at 250 (at
230 they collided); "OFF" from 250 ends near 287, clear of the notes at 310;
24 note glyphs from 310 end near 610. Rows: 5 visible at 34 px from y 108;
title at (32,52); footer at y 396. The music page shows 9 of 16 slots at 30 px
(no notes panel beside it); 9 is what the text budget allows.

The notes panel draws the selected mod's `.notes`, looked up through
`CGecko_NotesForOption(optionAddr)` keyed on the row's option word (only in
DOL-baked builds; the gecko stub returns 0 and the panel is simply empty).
`DrawNotes` word-wraps to 24 glyphs per line, honours an explicit `\n`, caps at
9 lines (past that the text runs into the footer, and every line costs a text
slot), and trims with "..." when there is more. Nine lines of 24 is ~200 glyphs.
Avoid `_` in a note: the font has no underscore. Wrapping one block per line
rather than through `WriteTextWrapped` is a claimed-RAM decision, see
`docs/text_engine.md`.

A row is `{label, addr, onValue, page}`: A writes `onValue` when the word is 0
and 0 otherwise; a row whose page is not the options page opens that screen
instead (its addr is still the notes key). The music page's Left/Right step
tracks through `MusicTrackStep`, which skips absent files; a red value means
"configured but the file is not on this disc" (only reachable from an ini code
or a stale word). Both pages live in the same scene function because the
dispatcher only calls us once; a second scene would need a second screenCode.

Options Menu claimed RAM: `0x802EB000` saved draw end, `0x802EB004` bg magic,
`0x802EB050` text buffer (1240 B), `0x802EB528` frame clock, `0x802EB52C` page,
`0x802EB530` music ScreenList (16 B), `0x802EC308` magic (`0x0D71104`),
`0x802EC30C` options ScreenList (16 B).
