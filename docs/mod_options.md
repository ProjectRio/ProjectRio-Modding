# Mod options: the toggle words and how the pack gates a mod

Used by: `RioModPack/ModOptions.h`, `RioModPack/RioModPack.c`,
`RioModPack/Gecko Codes.c`, `RioModPack/OnlineMenu.h`.

Related: `docs/menu_scenes.md` (the Options menu scene that flips the words),
`docs/custom_music.md`.

## The option block

One WORD per user-toggleable mod option in a fixed block of claimed free memory
(see `ClaimedFreeMemory.h`): `MODOPT_BASE = 0x802EB010`, `MODOPT_MAX = 16`
words, `MODOPT_ADDR(id) = base + id*4`. Sized well past `MODOPT_COUNT` so new
options never move the block: an option's id IS its offset, so moving the base
or renumbering an id would silently point a half-rebuilt mod at the wrong flag.
Ids are append-only.

| id | option |
|----|--------|
| 0 | `MODOPT_WIDESCREEN` |
| 1 | `MODOPT_CPU_SPRINT` |
| 2 | `MODOPT_INSTANT_RNG` |
| 3 | `MODOPT_DUPLICATES` |
| 4 | `MODOPT_SUPERSTARS` |
| 5 | `MODOPT_MUSIC` |
| 6 | `MODOPT_GECKO` |
| 7 | `MODOPT_NIGHT_MARIO` |
| 8 | `MODOPT_SWING_SKIP` |

Only the mods RioModPack actually ships get a row in the Options menu. A row
for a mod not in the pack is a switch wired to nothing; the other ids are
reserved and must not be renumbered.

**Why words and not bytes.** A gecko conditional compares either 32 bits
(`20`) or 16 bits with a mask (`28`). One option per word means a gate is just

    202EB010 00000001      // if the word at 0x802EB010 == 1

with no mask or shift arithmetic, which is what cgecko's `CGECKO_GATE_ADDR`
emits and what you would hand-write to gate a raw .ini code. Packed bytes would
need a different mask per option depending on alignment.

**Defaults and the sentinel.** Every option starts at 0 and 0 means OFF, which
is right for a mod. `MODOPT_GECKO` is the exception: it does not switch a mod
on, it switches OFF the emulator's gecko code handler, so 0 has to mean "leave
the handler alone" and that is the ON state on the menu. A zeroed block is
therefore indistinguishable from "user turned gecko codes off", and the code
enforcing that toggle runs from boot, long before the Options screen has ever
run. So the block carries a sentinel in its LAST word (index 15,
`'OPT1'` = `0x4F505431`, bump if defaults change): seeing it means
`ModOptions_ApplyDefaults` has run at least once. `ModOptions_Reset` clears the
sentinel with everything else so a reset re-seeds. The sentinel costs the top
word, so options may run 0..14.

The block is NOT saved anywhere: it is reset once per console session (the
first time the Options screen runs, guarded by a magic word at `0x802EC308`) to
the defaults. Persisting it to the memory card is a later stage.

`ModOptions_Reset` zeroes through a `volatile` pointer on purpose: a plain
zeroing loop is exactly the shape GCC rewrites into a `memset` call, and a gecko
payload has no libc.

## How a mod is gated

The mods themselves never include `ModOptions.h`. A mod in `Gecko Codes/` is
written as if the pack did not exist, and `RioModPack.c` wires the toggle in
from OUTSIDE, around the `#include`, using CGecko's two hooks:

| macro | effect | for |
|-------|--------|-----|
| `CGECKO_GATE_ADDR = MODOPT_ADDR(id)` | the code is not RUN while off: cgecko wraps the WHOLE code (every hook in the file) in a gecko `20` conditional | fire-and-forget mods |
| `CGECKO_ACTIVE = ModOptionOn(id)` | the code runs and restores itself while off | mods that patch game code or state |
| `CGECKO_OPTION_ADDR = MODOPT_ADDR(id)` | does not gate anything; names the option word this mod's `.notes` describe, so the Options row can look them up (`CGecko_NotesForOption`). Defaults to `CGECKO_GATE_ADDR` | any mod without a gate |

Standalone, a mod's `CGECKO_ACTIVE` is the constant 1 and its gecko build
carries no trace of the pack. With no `CGECKO_GATE_ADDR` set, the gate fields
are 0 and cgecko emits the plain code.

The gate is a `20` around the whole code, not an early return inside the body.
That matters: a gated-off C2 is never applied, so its `.instruction` never runs
either and the game is left genuinely stock. It is the only way to switch off a
REPLACE hook (`.instruction = "blr"`), and it works for raw `ASM()` codes,
which have no C body to return from.

Only pack-native code under `RioModPack/` (the Options UI, Gecko Codes, Custom
Music) reads the option words directly.

**Adding a mod:** give it an id in `ModOptions.h`, add a row to `s_options` in
`Options Menu.c`, and add the include with its gate lines to `RioModPack.c`.

### Why each pack member is wired the way it is

- **Options Menu.c**: never gated; it is how you reach the toggles.
- **Online Menu.c**: after Options Menu.c; shares its background blank/restore
  pair and its watchdog.
- **Duplicate Characters**: self-gates on `CGECKO_ACTIVE`. It patches game
  code, so it has to keep running while OFF to put the original instructions
  back; a code gated away entirely can never undo itself. `CGECKO_OPTION_ADDR`
  is set so its notes are reachable (the notes table keys on the gate and this
  mod has none).
- **Gecko Codes.c**: same reason (it patches the emulator's handler entry and
  must restore it). It is also the one option ON by default, because "off" here
  switches OFF something a user enabled outside this build.
- **Widescreen**, **Nighttime Mario Stadium**: plain `CGECKO_GATE_ADDR`.
- **Skip First Swing Frame**: two hooks that want different wiring. The swing
  hook is raw ASM injected into game.rel with no C body to test a condition in,
  so it is gate-wrapped (the gate decides whether the branch is installed when
  game.rel loads; flipping it mid-match takes effect at the next game load). The
  instruction patches edit game code and keep running while off to put it back,
  so they self-gate on `CGECKO_ACTIVE`, and they check that the ASM hook is
  really installed before applying, so a mid-match toggle cannot leave the swing
  animation with nothing to arm it.
- **Custom Music**: configured, not switched (sixteen slots; "off" is every
  slot on Default). NOT gated: gating it away would stop it handing the audio
  back when a match loads. Only `CGECKO_OPTION_ADDR` is set.
- **Dictionary Replaces Menu Music**: `CGECKO_ACTIVE` =
  `MusicSlot(MUSIC_SLOT_MENU) == MUSIC_DICTIONARY`; not gate-wrapped because it
  edits fx 484's layer id and the menu music volume and must restore them.
  `DMM_SCREENS` extends its main-menu set to screens 5, 6 and the Online
  screen. See `docs/custom_music.md`.

## The Gecko Codes toggle

RioModPack ships as a DOL: its mods are branches baked into main.dol and a
per-frame hook at `0x80009404`, none of which involve the gecko code handler. A
user can still enable ordinary gecko codes on top in Rio or Dolphin; those go
through the handler. This toggle switches that handler, and only that handler.

**How the handler is entered, and why there is nothing to nop.** A normal gecko
setup patches a `bl codehandler` over some instruction in the game's frame loop.
Dolphin does NOT do that. In `Core/GeckoCode.cpp`, `RunCodeHandler` builds a
stack frame by hand and sets

    LR = HLE_TRAMPOLINE_ADDRESS;                 // 0x80002FFC
    pc = npc = ENTRY_POINT;                      // 0x800018A8

a "phantom branch-and-link" driven from the host side once per frame. There is
no hook instruction in the game to put back.

So the lever is the handler itself: ordinary PPC code at
`0x80001800..0x80003000`, entry point `INSTALLER_BASE + 0xA8 = 0x800018A8`,
which stock reads `9421FF54` (`stwu r1, -0xAC(r1)`). Writing a `blr`
(`4E800020`) there makes the phantom call return immediately and no code in the
list is walked. Returning is safe: `HLE_Misc::GeckoReturnTrampoline` restores
r1, npc, LR, CR and FPR0-13 entirely from the frame the HOST wrote, so it does
not care whether the handler ran; the handler's own `stmw`/`lmw` never happens,
which is fine because it never saved anything.

**Detecting an installed handler.** The handler writes `MAGIC_GAMEID`
(`0xD01F1BAD`) to its first word (`0x80001800`) on install, and
`HLE_Misc::GeckoCodeHandlerICacheFlush` increments it once per frame for the
first five frames. So the word reads MAGIC..MAGIC+5 whenever a handler is
installed; anything else means none (no codes enabled, or a build outside an
emulator). Never write to `0x800018A8` in that case: with no handler there it
is just some other code's memory.

**Why not blank the code list instead.** The list base is recoverable (Rio
writes it into the handler as a lis/ori pair at `0x80001904`/`0x80001908`, so
`base = ((*0x80001904 - 0x3DE00000) << 16) | (*0x80001908 - 0x61EF0000)`), and
an F0 terminator at its head would also stop every code. But that means saving
and restoring 8 bytes of somebody else's data and tracking a base that moves
with the list; one conditional word at a fixed address is smaller and cannot
get out of sync.

**Scope.** This disables EVERY code in the handler's list, which under Project
Rio includes Rio's own built-in codes, not just user-added ones. The list is a
flat run of code words with no record of origin, so nothing can tell them apart
at runtime. Anything of Rio's that depends on its codes (stat tracking and the
like) is off while this is off.

The patch runs `.state = MSSB_ALWAYS` (the handler is live during a match too)
and re-arms itself: Dolphin re-installs the handler whenever it reloads the
code list, which puts the stock word back, and the next frame patches it again.
Both directions are the same conditional write with the arguments swapped, so
each is idempotent.

## The Online screen code (`OnlineMenu.h`)

`screenFuncTable[2]` and `[3]` are the game's "removed step" assert stub
(`0x806402B0`: an `OSPanic` no stock code reaches). The Online mod replaces that
handler, so `ONLINE_SCREEN_CODE = 2` becomes the Online screen. It is reached
only through the mod's own reroute of the Options transition, never by the
game, and never has to survive a menu reload: leaving it puts the main menu
back exactly the way the Options screen does.

Two other pack members recognise it: the Options Menu's background watchdog
(which un-blanks the UI whenever "our" screen is not current) and the
Dictionary music swap's list of main-menu screens. Both include the header
rather than the mod.
