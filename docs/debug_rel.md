# debug.rel: the leftover developer debug menu

MSSB ships an unused third REL, `debug.rel` (the repo used to call it
`challenge.rel`; the decomp renamed it). It is module id 1, the third LZ blob in
`aaaa.dat`, and nothing on the retail disc references it. Loading it in place of
`menus.rel` brings up the game's developer test-menu suite.

Used by: `Gecko Codes/Global/Debug Mode.c` (boot-time load + text overlay +
in-suite fixes) and `Gecko Codes/Menu/Debug Mode From Main Menu.c` (launch it
from the Records button). The loader itself is described in `rel_loader.md`.

## Module identity and the file table

All three RELs are LZ blobs inside `aaaa.dat`, described by a 16-byte-per-entry
file table at `0x800E8AA8` (`{lzParams, flag|decompSize, offset, compressedSize}`):

| entry  | module    | words                                       |
| ------ | --------- | ------------------------------------------- |
| +0x00  | menus.rel | `0000040B 401027E4 00000800 0005A818`       |
| +0x10  | game.rel  | `0000040B 402220F8 0005B800 000F4450`       |
| +0x20  | debug.rel | `0000040B 4005912C 00150000 000271C0` (referenced by nothing) |

Overwriting the menus entry's words 1..3 with debug's values (word 0 is
identical) makes every menu-REL load pull debug.rel instead:

    040E8AAC 4005912C
    040E8AB0 00150000
    040E8AB4 000271C0

When linked into the menu slot, debug.rel's `.text` starts at `0x8063F094`.

## Why no glue is needed (traced 2026-08-28)

Earlier versions of the boot-time code added post-prolog glue, a VI-callback
frame pump, forced node sequencing and a pre-launch of entry 0's UI framework.
None of it is needed; the stock frame loop already brings the module up:

* debug.rel's `_prolog` (`0x806401A0`) calls `setDrawingListHeadFunctions`
  (`0x806401DC`), which writes that address into slot +0 of all three drawing
  banks (`0x803C7A24 + i*0x80`).
* `RunDrawScripts_with_stack_variables` (`0x800B0CB8`), called from main every
  frame, walks the current bank starting at slot +0, so `0x806401DC` is called
  once per frame by the stock loop.
* `0x806401DC` is a self-replacing initializer: it zeroes the current node's
  scratch (+0x14/+0x16), resets the drawing lists (`0x800B0BE8`), re-registers
  the DOL base draw fns (`0x80009144`), sets the clear colour, and overwrites
  its own node slot with the debug menu's frame main (`0x80640734`). From the
  second frame on the debug menu runs in its place.

The old glue called `0x806401DC` by hand from inside `handleLoadingProcess`,
where the "current node" is the loader's node. That installed the debug menu
over the loader itself and reset the lists mid-load, which is what the frame
pump and node-repair code were compensating for. Verified live: module id 1
links, the bank head becomes `0x80640734` on the next frame, steady 60 VI/s.

The old "swap alone hangs on a turquoise screen" reading was wrong the same
way: a flat clear colour is what debug.rel draws (its `.data +0xA8`) once its
head fn has run. The menu was up the whole time; it just draws no text.

## The top-level selector (0x80640734)

* D-pad Up/Down move the selection (index in the bank node's +0x16)
* A launches the selected entry through the 14-entry jump table at
  `0x80670CF8`: each entry calls `insertGraphicDrawingFunction(script, 10)` and
  swaps the node fn to the sub-scene runner `0x806409DC`
* B+Y held -> alternate mode `0x8063F620`
* START signals "menu finished" to the loader, which tears debug.rel down and
  boots game.rel: expect a crash

Input is the DOL-polled menu-input struct at `0x803C77B8` (+0 held, +2 newly
pressed, +4 held/repeat), so controls need no extra plumbing.

Entry names (from live testing and the decomp's unit map):

| idx | rep unit | name                        | note                          |
| --- | -------- | --------------------------- | ----------------------------- |
| 0   | rep_0138 | Stadium Viewer              | hidden picker, see below      |
| 1   | rep_0250 | Countdown                   | runs, draws nothing           |
| 2   | rep_0610 | Char Viewer                 | assets not on disc            |
| 3   | rep_7A28 | Particle Editor             |                               |
| 4   | rep_74A0 | Model Viewer                | needs the SKN null guard      |
| 5   | rep_7730 | Sprite Viewer               |                               |
| 6   | rep_02A8 | Sound Test                  |                               |
| 7   | rep_7730 | Sprite Unit                 |                               |
| 8   | rep_7BA0 | Cutscene Player             |                               |
| 9   | rep_7BF0 | Bat Sim Menu                |                               |
| 10-13 | -      | no-ops                      |                               |

## Why the suite has no text

Two separate holes, neither a missing asset:

**(a) The selector draws nothing.** `0x80640734` is pure input handling. The
devs drove it blind. Verified by capturing the XFB out of emulated RAM: flat
clear colour, no geometry, no textures bound.

**(b) The in-scene menus' renderer was compiled out of the retail DOL.** The
MenuItem trees (labels "MAIN MENU", "BAT MENU", "SIM NUM", "LENGTH", "FIRST
POS", "VEL X" ... in debug.rel's `.rodata`) call the DOL to draw themselves:

    80048bec: lwz r4,8(r3)    ; rows = state->rowsPerPage
              lwz r0,4(r3)    ; count = state->count
              cmpw/bge/mr     ; n = min(count, rows)
              mtctr r4
              cmpwi r4,0
              blelr
    80048c0c: bdnz 80048c0c   ; <- the per-row loop, body GONE
              blr

Sibling debug helpers went the same way (`0x80026130` is a bare `blr`), while
the menu's input handler (`0x80048820`, 0x3CC bytes) survived, which is why
those menus respond to the d-pad while showing nothing. Both are called only
from debug.rel (zero call sites in merged DOL+menus.rel and DOL+game.rel), so
hooking the stub cannot affect normal play.

Caveat (measured live): in rep_7A28 (entry 3) the struct exists but is never
populated: count = 0 and items = NULL, while rowsPerPage = 16 and wrap = 1 are
real. Its call is gated on `currentDrawingItem->+0x34` bit 0x8, which nothing
observed sets. So that menu stays empty even if the gate is forced.

The state struct `fn_80048BEC` receives (read off the input handler):

| offset | type      | field        | meaning                                |
| ------ | --------- | ------------ | -------------------------------------- |
| +0x00  | u32       |              | unused                                 |
| +0x04  | s32       | count        | number of items                        |
| +0x08  | s32       | rowsPerPage  | visible rows (0x10 in the suite)       |
| +0x0C  | s32       | scroll       | index of the top visible row           |
| +0x10  | s32       | index        | selected item                          |
| +0x14  | s32       | wrap         | selection wraps at the ends            |
| +0x18  | MenuItem* | items        | 0x2C bytes each; +0x04 = char* label, NULL label terminates |

MenuItem value rows: +0x1C = pointer to the row's live value (0 on navigation
rows); +0x00 = row type, 0 = plain integer (SIM NUM, 1..100), 1 = fixed point
scaled by 100000 (rep_7BF0 divides every one by 100000.0f), printed as n.nn.

### Drawing text under debug.rel

The DOL loads the text banks and font pages during boot states 0-3
(ARAMTransfer of table entries `0x800E8AD8/AE8/AF8`, then `initTextRendering`
at `0x80010FA0`), before any REL loads, so the font is resident under
debug.rel. The ScreenText pool render pass is a DOL base draw script
(`fn_80009094` -> `maybeProcessUIUpdates 0x80035168` -> `DrawTextOnCondition
0x80010F2C`), registered by `fn_80009144`, which debug.rel's head fn calls. The
overlay just fills blocks in the pool.

`Include/Rio/ScreenText.h` is not used because it compiles to ~470 gecko lines
per hook and the target Dolphin build had only 3256 bytes of code-list space
("Too many GeckoCodes! ... only 3168 remain"). The overlay is the same
block-filling recipe with the formatter dropped. Draw exactly rowsPerPage rows
(16 + a cursor marker): drawing fewer makes the cursor walk off the bottom
before the game scrolls, since it scrolls on its own page size.

A scene's own node is not the bank head: the head runs the sub-scene runner
(`0x806409DC`) and the scene sits further down the chain (node +0x00 = fn,
+0x08 = next). The live list is five nodes deep.

### Per-scene state the overlay reads

| symbol             | address      | meaning                                          |
| ------------------ | ------------ | ------------------------------------------------ |
| g_loadedModuleId   | `0x8063EFC0` | 1 = debug.rel (plain heap before any REL loads; can transiently read 1 during the intro, so gate on the bank fn too) |
| DRAW_BANK_0        | `0x803C7A24` | banks are 0x80 apart                             |
| g_currentBankIndex | `0x803CC1B0` | u16                                              |

Entry 6, Sound Test (rep_02A8): `fn_1_A348` (`0x806493DC`) is the top level,
d-pad up/down move a cursor mod 5 (`0x806E1B98`, s8), A swaps the node fn for
`lbl_1_data_1C8C[cursor]`. Labels: 19 `const char*` at `0x80672298`, [0..4]
are the items, [5] "VOICE NO " is the caption the devs never wired up.

| item | scene fn                          | value                                  |
| ---- | --------------------------------- | -------------------------------------- |
| 0    | Voice Test `fn_1_BFB0` `0x8064B044` | s16 `0x8069B176`, wraps at 0xE9       |
| 1    | Voice Test 2 `fn_1_BC00` `0x8064AC94` | s16 `0x8069B174`                     |
| 2    | Training Se Test `fn_1_A464` `0x806494F8` | s16 `0x806727E4` (.data)          |
| 3    | SE Test `fn_1_B5B8` `0x8064A64C` | mode u8 `0x8069B100` (Y->0, X->1), row u8 `0x8069B101` (0..3), values s16 `0x806727E2` (wraps 0x151..0x1B6), `0x8069B102`, `0x8069B104`, `0x8069B106` |

Voice tests: left/right step by 1, Y+left/right by 10, A plays, B back. SE
Test: rows 1 and 2 are indices into id tables (`0x806723CC`, 48 ids;
`0x8067242C`, 24 ids), row 0 plays its value directly, row 3 goes through
`fn_80062890`. X sets mode 1, which makes the scene return before it reads any
button including B, so it looks like a lock-up until Y is pressed.

Entry 0, Stadium Viewer (rep_0138): `fn_1_6848` installs `fn_1_6DEC`
(`0x806458DC`) as an invisible picker: d-pad up/down move `0x806DF604` (u8
0..6, index order = STADIUM_ID), A commits and loads that stadium (one-way; B
does not return from the viewer `0x80645EA8`).

## The GX-thread OSPanic

The suite calls GX from a thread that is not the current GX thread, so two DOL
texture setters trip a developer assert: "sub.c: gOz_GXSetTexture was called
in thread that is not current GX thread." Both guards have the same shape and
the panic is the only thing in the failure branch, so NOPing the call falls
straight through to the label the passing case branches to:

    800241ac  bl GXGetCurrentGXThread      (gOz_GXSetTexture, sub.c:674)
    800241b4  bl OSGetCurrentThread
    800241bc  beq 800241d8                 <- passing case skips the panic
    800241d4  bl OSPanic                   <- NOP this
    800241d8  bl GXClearVtxDesc            <- both paths resume here

    8002442c/80024434/8002443c/80024454/80024458   the same in
                                 SetDisplayStateTexture (sub.c:574)

Both are NOPed (`0x800241D4`, `0x80024454`) because the shared message names
gOz_GXSetTexture either way. Inert in normal play: retail never calls GX
off-thread.

## Entry 4 (Model Viewer): SKNIt NULL skin

The viewer draws an actor whose geometry type byte says SKINNED (0x40) while
its skin/bone array was never attached:

    800b3b0c  lbz r0,6(r5)     ; geometry type
    800b3b14  bne 800b3b2c     ; 0x40 -> skinned path
    800b3b18  lwz r3,24(r25)   ; actor->+0x18   ok
    800b3b1c  lwz r4,124(r25)  ; actor->+0x7C   NULL
    800b3b24  bl  SKNIt        ; SKNIt entry does lbz r3,6(r4) -> DSI, DAR 0x6

A NOP is not enough because the pointer is really used, so the call at
`0x800B3B24` is replaced with a guarded call to SKNIt (`0x800BF89C`) that
skips the draw when r4 is not a valid pointer. r0 is dead there. SKNIt's other
caller (`0x800B2DA4`) is left alone; it has not been seen taking a NULL.

## Entry 1 (Countdown): two faults, three word patches

`fn_1_9AB0` (.text 0x9AB0 -> `0x80648B44`) is entry 1's frame function, a
three-phase state machine on the bank node's +0x10 with its state in one .bss
block (`lbl_1_bss_2FA8` -> `0x8069B0C8`, "S"):

    phase 0  init:  seed S, S->0x1C = 0, phase = 1
    phase 1  load:  if (S->0x1C > 0xB) { phase = 2; return; }
                    if (diskReadRelated(&table[S->0x1C], S->0x1C)) S->0x1C++;
    phase 2  run:   fn_1_97E4() reads input, then dispatch on S->0x02

**(a) The load loop runs one entry past its table.** The descriptor table is
`lbl_1_data_AB0 + 0x45C` (`0x80671A4C`), 16 bytes per entry in the REL-table
shape, and holds eleven entries, not twelve:

    [ 0] 0000040B 4005BDE4 0EAB5000 000161A0
    ...
    [10] 0000040B 4000785C 0EAE1800 000023A8
    [11] 01000003 1B1A1918 17161514 13121110   <- the next data object

The twelfth iteration hands neighbouring data to ARAMTransfer, and
diskReadRelated's relocation pass walks off the returned buffer:

    8000B9C8  lwz r0,8(r3) ; add r0,r0,r3 ; stw r0,8(r3)   r3 = 0x80775D40
    8000B9EC  lwz r8,8(r3)                                 r8 = 0x00EEBA80
    8000B9F0  lwz r5,0(r8)                                 <- DSI, CPU stops

Fix: clamp the loop compare at .text 0x9B8C from 0xB to 0xA
(`2C04000B` -> `2C04000A`). An earlier theory ("pressing A re-runs the load
over an already-relocated buffer") was wrong: the twelve DOL resource slots
(`0x803C4BE0`, 0x3C stride) are filled exactly once.

**(b) The scene's graphics element is never assigned.** A is handled at
.text 0x98A8 (in `fn_1_97E4`): it cycles S->0x07 and then calls
`removeGraphicsElementFromScene(S->0x24)` with S->0x24 = 0, DSI on
`lhz r7,0x14(r3)`. The mode-0 body at .text 0x9C64 dereferences it again
itself, so the pointer has to be real. Both sibling scene functions open with
`stw currentDrawingItem, 0x24(S)` (`fn_1_8F34` .text 0x8F80, `fn_1_9380`
.text 0x93C4); `fn_1_9AB0` holds currentDrawingItem in r29 but never stores
it. Two free words in phase 0 supply the store:

    .text 0x9B6C  lwz r3,0(r3)     -> stw r29,0x24(r31)   (80630000 -> 93BF0024)
    .text 0x9B80  sth r0,0x10(r3)  -> sth r0,0x10(r29)    (B0030010 -> B01D0010)

Verified live: entry 1 loads, reaches phase 2, survives A indefinitely, and
addGraphicsElementToScene registers its four sprites. It still draws nothing
(same "invisible scene" state as entries 0, 3, 6 and 8).

These are REL addresses, so they are written per frame from C rather than as
04 codes: a static write would land in whatever module occupies the range,
and no gecko conditional can express "module id is 1 and the word still holds
the expected instruction". The writes are followed by `DCFlushRange`
(`0x8006E894`) + `ICInvalidateRange` (`0x8006E94C`) because this is
self-modifying code.

## removeGraphicsElementFromScene corrupts the table on an empty element

The DOL keeps one flat table of 0x360 graphics slots (`graphicsRelatedArray`,
`0x80371C30`, 8 bytes per slot). A drawing node that owns sprites records its
first slot at +0x14 and the count at +0x16. `removeGraphicsElementFromScene`
(`0x80034CEC`) drops that run and compacts the table, renumbering survivors:

    80034D5C  lwz/stw 0(r5)->0(r3), 4(r5)->4(r3)   ; shift a slot down
    80034D6C  lwz r4,4(r3)
    80034D78  sth r6,0x14(r4)                      ; elem->0x14 = its new index
    80034D84  addi r6,r6,1

It never checks the count. Called with +0x14 = +0x16 = 0, src == dst and the
shift moves nothing, but the loop still runs all 0x360 iterations and stamps
r6 into +0x14 of every element it passes. Any element spanning several slots
ends up with its last index in +0x14, so the next remove() on it drops the
wrong run; the damage surfaces later, in another scene. Reached from entry 1's
mode-0 body (.text 0x9C64) and its B handler (.text 0x98F4). Found by cycling
entries: 4 -> 1 -> 4 -> 1 wedges on the second visit to entry 1.

Fix: an ASM guard at function entry (`lhz r0,0x16(r3); cmplwi r0,0; beqlr`)
re-issuing the overwritten `lis r4, 0x8037`. ASM rather than C because a C2
in C cannot return from the function it is injected into; r0 and CR0 are dead
at a function entry.

## Entry 2 (Character Model Viewer): diagnosed, not fixable

It loads characters by path, and those paths exist nowhere on a retail disc.
`fn_1_16978` (.text 0x16978 -> `0x80655A0C`) is a 30-state loader dispatched
through `jumptable_1_data_F810` (`0x80680350`); twelve of its states call
ARAMTransfer with a 16-byte descriptor from `lbl_1_data_2398` (`0x80672ED8`,
nineteen per character). These lead with a pointer to a name in debug.rel's
`.rodata` (`char/nin00/model0.dat`, `char/nin00/motb.dat`, ...) instead of the
LZ parameter `0x0000040B`. The string "nin00" does not occur anywhere in the
retail image; the shipped archive is offset-addressed only, so every lookup
resolves to offset 0.

Measured: the loader reads ZZZZ.dat (DVD entry 22) from offset 0, the LZ
decode yields nothing, and manageFileReadingProcess's stuck-detector fires:

    800A6BBC  lbz r0,0x364(r31)   ; decoder error code -- ZERO here
    800A6BC8  lbz r0,0x6d8(r30)   ; input exhausted     -- set
    800A6BD4  lwz r0,0x6d4(r30)   ; bytes pending       -- zero
    800A6BE0  DVDCancel + OSCancelThread x2 + OSCreateThread + OSResumeThread
    800A6C54  OSReport("Decode Error Retry : EntryNo%d %x Err:%d")

Every character change tears down and rebuilds the DVD-read and decompressor
threads; after six the console wedges with an FP-unavailable exception at
`fn_1_135C0 + 0xC`. Two bypasses were rejected after live testing: forcing the
loader straight to its terminal state 29 (`0x80655A3C` -> `li r6,29`) faults on
the buffer pointers in `hugeAnimStruct+0xC0C`; NOPing the loader install
(`0x80653FD4`) buys one more round of input and faults elsewhere.

## Still open: B kills entries 1, 4 and 9

All three die with LR = `0x800B0D08` (inside
`RunDrawScripts_with_stack_variables`) and SRR0 = CTR = an instruction word
(`0x2C000004`, `0x3CA00000`, `0x38C00004`) with SRR1 bit 0x40000000 set: an
ISI, the frame loop `bctrl`d into a drawing node whose function slot is not a
function. One shared teardown bug. Entries 0, 2, 3, 5, 6, 7 and 8 survive B.

## Launching from the Records button (Debug Mode From Main Menu.c)

Instead of swapping the table at boot, the Records hook does the same swap and
then makes the loader re-load the menu slot rather than moving on to game.rel:

1. The main-menu dispatch is `li r3,8; bl changeScreenVariables` at
   `0x80641848/0x8064184C` (menus.rel .text 0x27B4). The `bl` is hooked and
   dropped. The hook writes the three table words, raises the loader's
   "finished" flag the way menus.rel itself does when it hands back
   (`parent->+0x10 = 1` + `removeCurrentDrawingItem`, see menus.rel .text
   0x1A3B4), and clears the Rio scene id (`0x800E877C`) so no menu-gated code
   keeps writing into the slot debug.rel is about to occupy.
2. `sth r0, 0x18(r29)` at `0x80009C0C` is the only place loader state 0xA
   advances to 0xB (load game.rel). When a launch is armed it writes 8 (reload
   the menu slot) instead.
3. Loader state 9, the instruction after the freshly linked REL's prolog
   `bctrl` (`0x80009BA4` = `li r0, 0`), zeroes the DOL's 2D element pool and
   frees the text blocks (see below).

The arm/loading latch lives at `0x802EC2B4` (`0x0DEB0601` armed,
`0x0DEB0602` loading). Nothing restores the table afterwards, so START in the
debug menu still tears debug.rel down into game.rel and crashes.

### Why the menu stayed on screen (verified 2026-09-01)

The menu's 2D graphics are not drawn by menus.rel. They are element records in
the DOL's own pool, `menuGraphicsStructures` (`0x8039C3E0`, 864 x 0xC0 bytes,
+0x54 != 0 = in use), drawn every frame by the DOL base draw script
`maybeProcessUIUpdates` (`0x80035168`), which debug.rel re-registers through
`fn_80009144`. Neither the loader teardown nor the REL epilog frees them, and
the main menu's exit fade (scene-manager mode 2, a full-screen black element)
is one of them. At boot the pool is all zero, so the launch code zeroes the
pool (and the ScreenText blocks via `text_freeAllBlocks`, `0x8000FE54`).

Timing matters: wiping in the teardown frame crashed (DSI in `fn_800B27DC`
under maybeProcessUIUpdates) because the DOL scene manager is still the bank
head for that frame and the element pass still walks the menu's records. Right
after debug.rel's prolog (loader state 9, `0x80009BA4`) the prolog has made its
own init the bank head, and that init resets every drawing struct before any
draw pass runs, so the wipe is safe.
