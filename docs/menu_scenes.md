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
addresses are in `Include/Rio/MenuScene.h`.

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
