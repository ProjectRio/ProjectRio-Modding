# Menu screen dispatch: screenFuncTable and hijacking a scene

How a menu "scene" is dispatched each frame, and how a mod replaces or
augments one. `menu_scenes.md` covers the 2D record pool a screen draws with;
this is the control-function side. Used by:
`Gecko Codes/Global/Custom Menu Scene.c` (hello-world replacement of the
Records scene), `Gecko Codes/Menu/Dictionary Scene.c` (augmenting the
Dictionary scene), and `Include/Rio/DictionaryReroute.h`.

## The dispatch

MSSB menus are a per-frame jump table. `currentScreenFunctionChooser`
(menus.rel `0x8064026C`) does:

    sc  = menuControlStruct->screenCode;      // *(0x803CBBCC) + 2, u16
    fn  = screenFuncTable[sc];                // table @ 0x806D71CC
    fn();                                     // called every frame

So a scene is just `screenFuncTable[screenCode]()`. Screens change through
`changeScreenVariables(code)`. Known codes:

| code | screen                                        | handler        |
| ---- | --------------------------------------------- | -------------- |
| 5    | main menu                                     |                |
| 6    | Options                                       |                |
| 7    | Dictionary (dormant, present in the JP build) | `0x80693C44`; its menuProcess sub-table is at `0x806F7934` |
| 8    | Records                                       | `0x8069D198`   |
| 9    | captain select                                | `0x8065201C`   |
| 10   | team select (the draft)                       | `0x80650144`   |

The main menu's Records button is `li r3,8; bl changeScreenVariables` at
`0x80641848/0x8064184C` (menus.rel .text 0x27B4).

Every one of these addresses is menus.rel code, only valid while `rel == 4`.
A hook on them must carry `.state = MSSB_MENU`: at `rel == 5` the same address
is game.rel and must not be hooked.

## Replacing a scene cleanly (Custom Menu Scene)

A cgecko C2 runs `backup -> payload -> restore -> [.instruction] -> branch
back`. Hooking the handler's first instruction and overriding the re-executed
instruction with `blr` returns to the dispatcher's `bctrl` the instant the
payload finishes, so the stock body (which begins `stwu r1,-80(r1)`) never
runs and there is no stack leak. `screenFuncTable[8]()` then runs only the
mod's code and returns normally.

To retarget the unused Dictionary scene (7) instead: change the hook's
`.address` to `0x80693C44` and add a companion menu-gated code that reroutes
the Records button, a `# State: Menu` .asm with the single instruction
`li r3, 7` at `0x80641848` (cgecko emits it as the gated 04 write
`04641848 38600007`). Dictionary is dormant, so replacing its handler keeps
the real Records screen intact; its menu-framework entry transition is
unexercised, which is why the first proof used the known-good Records slot.

## Augmenting a scene (Dictionary Scene)

Hook the handler's first instruction with `.instruction = "stwu r1, -48(r1)"`
(the stock handler's own first instruction): the overlay runs at the top, then
the stock scene (background, its registered draw callbacks, state machine,
input, exit) carries on and the mod only adds elements. Stock element draw
callbacks for the Dictionary scene live at `0x80693A48`, `0x80692930`
(menus.rel) and `0x8005295C`, `0x80053FE8` (main.dol).

The stock scene uses A/B/dpad, so Y is free for a mod toggle. `newInput` on
`Static_Stats_Tables.controllerInputs` is edge-detected by the menu's input
gather (a bit is set only on the frame the button is first pressed).

## cgecko hazards in a scene payload

**No payload statics.** cgecko accesses payload `.data` statics PIC-relative
through r31; a static read from inside a helper (e.g. `ScreenTextTick` reading
a custom frame counter) dereferenced a NULL base and crashed in testing (DAR
0x60 at `0x80002A48`). All scene state lives at absolute claimed-RAM addresses
instead (`0x802EC270` one-shot init sentinel, `0x802EC274` counter/toggle), and
ScreenText uses its default frame counter (`FrameCountWhileNotAtMainMenu`, an
absolute game var, valid on submenus). String literals are safe because their
pointers are computed in the entry and passed as arguments.

**Keep the C2 payload small** (well under the ~546-u32 cliff documented in
Boot Directly To Game). `SCREENTEXT_NO_FLOAT` drops the `%f` formatter and its
FPU save/restore; `TEXT_BUFFER_ADDR` (`0x802EC0E8`, 392 bytes) moves the glyph
scratch buffers into claimed RAM instead of embedding them in the payload.
