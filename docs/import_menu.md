# Imported in-service codes: menu / boot group

Engine notes worked out while importing Rio's hex-only codes. Anything here
that is not yet in the decomp is an upstream candidate.

## Boot scene dispatch (`Include/Rio/BootScene.h`)

`0x8063F964` (menus.rel, resident while `inningSetting.rel == 0`) is
`li r3, 1; bl 0x80640234` -- the boot walk's "go to scene r3" call. Every boot
code replaces the scene number there, so only one can be active:

| code | scene | extra |
|---|---|---|
| Boot To Main Menu | 5 | drives the memory-card load afterwards |
| Boot to Progressive mode | 0x11 (progressive-scan prompt) | |
| Boot to Minigames / Practice Mode / Toy Field | 4 | sets `g_d_GameSettings.GameModeSelected` (7 / 2 / 6) first |

The originals of the three scene-4 codes also zeroed r14/r15 (assembler scratch
hygiene); the C hook never touches them.

## Character select (`charSelectStruct`, 0x803C6028, size 0x94)

- `+0x18` s16[4]: each player's cursor square, -1 when the player has none.
  `characterIconsOnCSS` (0x800FDE84) maps square -> charID into `+0x20` s16[4].
- `+0x28` u8[36] (0x803C6050): one byte per grid square, reset to 0xFF by the
  loop at 0x800512D8. A non-0xFF value makes the square unpickable (believed to
  be the owning player; unverified). Bowser is square 11.
- 0x80050A54 / 0x80050AFC: the cursor-move right/left paths skip a square whose
  icon table entry is negative (unused); nop = the cursor can rest there.
- 0x803C77B8 + 0x20*player, byte +6: high byte of that pad's newly pressed
  buttons. Ban Characters tests it for exactly 0x04 (X and no Y/Start).
- The UI-record base 0x80371C30 is held in r0 across 0x80051AB8-0x80051BC0, so
  Ban Characters hooks 0x80051AA8 (r0 is reloaded by the replayed instruction)
  instead of the original 0x80051B3C. Same block, same per-player loop.

## gameSettings defaults (`Include/Rio/GameSettingsDefaults.h`)

The defaults block at 0x800498C0 fills `gameSettings` (0x803C5F04): `li r0, 2`
at 0x800498C4 is the innings index stored to +0x3E (0..4 = 1/3/5/7/9 innings);
+0x3F is mercy.

## Misc

- `constantList` (game.rel 0x807C10A8) +0xA: 8 charIDs, practice mode's default fielders.
- 0x80366148 u8[4]: minigame turns left per player. 0x8036600C u16: coin total.
- Music enable flag = byte 0x398 of the object at 0x800EF808, tested at
  0x80062AB0 (DOL) and 0x806CCCB0 (game.rel).
- 0x8086EF38, stride 0x10, 3 entries (game.rel bss): haze layer enable bytes.
- Never-cull sites: see docs/widescreen.md.
