# The DOL REL loader

How MSSB's DOL loads, links and swaps its three RELs, and where a mod can hook
that. Used by: `Gecko Codes/Menu/Debug Mode From Main Menu.c`,
`Gecko Codes/Global/Debug Mode.c`, `Gecko Codes/Global/Boot To Match.c`,
`Gecko Codes/Global/Instant Randoms.c`, `Gecko Codes/Rio Built-in/Game ID.c`.

## The REL file table (0x800E8AA8)

16 bytes per entry, `{lzParams, flag|decompSize, offset, compressedSize}`, all
three blobs inside `aaaa.dat`:

| entry | module    | words                                 |
| ----- | --------- | ------------------------------------- |
| +0x00 | menus.rel | `0000040B 401027E4 00000800 0005A818` |
| +0x10 | game.rel  | `0000040B 402220F8 0005B800 000F4450` |
| +0x20 | debug.rel | `0000040B 4005912C 00150000 000271C0` |

The loader reads the table verbatim: whatever an entry points at is what gets
loaded into that slot (`debug_rel.md` uses this to load debug.rel over menus).

## handleLoadingProcess (0x800097A0) and its node (0x80111300)

The boot/reload state machine runs on the fixed node at `0x80111300`, which
`RunDrawScripts` calls first every frame:

| offset | type | meaning                                                   |
| ------ | ---- | --------------------------------------------------------- |
| +0x10  | s16  | "finished" flag a REL raises to hand control back         |
| +0x14  | ptr  | the linked module                                         |
| +0x18  | u16  | loader state                                              |

States that matter:

| state | what happens                                                        |
| ----- | ------------------------------------------------------------------- |
| 0-3   | text banks and font pages (ARAMTransfer of `0x800E8AD8/AE8/AF8`, `initTextRendering 0x80010FA0`) |
| 7     | hands `&table[0]` (the menu slot) to ARAMTransfer (`0x800A70DC`)    |
| 8     | post-game "reload the menu slot": ARAMTransfer of table entry 0     |
| 9     | OSLink the menu-slot REL and `bctrl` its prolog (`module+0x34`); the instruction after the bctrl is `li r0,0` at `0x80009BA4` |
| 0xA   | parked while a REL runs, until +0x10 becomes 1                      |
| 0xA->0xB | teardown: `maybeLoadsGameSoundFiles`, `resetAllDrawingStructs`, `fn_80009144`, the REL's epilog, OSUnlink; then `sth r0,0x18(r29)` at `0x80009C0C` writes 0xB |
| 0xB   | load game.rel                                                        |
| 12    | OSLink game.rel and `bctrl` its prolog; the instruction after is `li r0,0` at `0x80009D04` |

So there are two link sites, and they fire for every REL that goes through
that slot: state 9 (`0x80009BA0`) for the boot-time load and the post-game
menu reload, state 12 (`0x80009D00`) for the menu -> match swap. A hook that
only cares about game.rel has to check the Rio scene id itself.

## The Rio scene id (0x800E877C)

The halfword every `MSSB_MENU` / `MSSB_GAME` gate tests: 0 boot, 4 menus.rel,
5 game.rel. It reads 0 under a boot-loaded debug.rel. The game (and every boot
code) sets it to 5 *before* requesting the swap; the loader reads it to pick
the file. That is why a hook at the link sites can test `rel == 5` to know
game.rel just linked.

A gecko `.state` gate only controls when a C2's branch is first written; it
never removes it. So a hook that must behave differently per REL has to check
in C regardless of `.state`.

## Requesting the menu -> match swap by hand

The force-swap boots (Instant Randoms, Boot To Match) skip the menu walk
entirely:

    inningSetting.rel = 5;          // 0x800E877C
    trigger_rel_change = 1;         // 0x80111310, the loader node's +0x10

Flipping `rel` only *requests* the swap; the teardown happens later in the
loading process, so menu-REL functions (`copyInfoToInMemRoster` and friends,
all at `0x8064xxxx`) are still callable on the same frame. A per-frame boot
code therefore cannot carry a `.state` gate (it has to see rel 4 and keep
running through the transition) and must gate on `rel` in C instead.

## Header ordering hazard

`Include/game/UnknownHomes_Game.h` must be included before
`Include/static/UnknownHomes_Static.h`: the static header defines
`inningSetting` as a macro and the game header has a struct field of that name
(`GameInitOptions`), which the macro would mangle if it came second.
