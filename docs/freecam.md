# Allow Freecam From External Programs

Used by: `Gecko Codes/Rio Built-in/Allow Freecam From External Programs.c`
(per-frame, always). Companion to Roeming's hitbox visualisation tools, which
drive the camera from outside the game.

## Decoding the original .ini

```
20052988 480003a0 / 04052988 800d8140      if patched -> restore original
202ec021 00000001                          endif; if word 0x802EC020 == 1
  20052988 800d8140 / 04052988 480003a0      if original -> patch
e2000002                                   end both
0401dcf8 38000007, 046aa4e4 38000001, 046ab8b4 38000001   unconditional
200bd9dc 7c831b78 / 040bd9dc 38600007      if original -> patch
206f7b7d 881a0093 / 046f7b7c 38000003      endif; if original -> patch
e2000001
```

A Gecko `20` conditional with an odd address is "endif, then if" on the even
address, so the toggle is the WORD at `0x802EC020` (== 1), not a byte at
`0x802EC021`. Restoring first and re-patching inside the toggle block makes
the net effect "patched iff the word is 1", which is what `RioPatch_Apply`
with the toggle as `on` does.

`0x802EC020` is also the start of the Write Text To Screen `ScreenText`
buffer claim in `ClaimedFreeMemory.h`; the two were never meant to run
together, but the collision is real and should be resolved by moving one.

## Patch sites

| site         | function                    | original -> patched                        |
| ------------ | --------------------------- | ------------------------------------------ |
| `0x80052988` | graphics_relatedToVsScreen  | `lwz r0, -0x7EC0(r13)` -> `b +0x3A0`: skip the block that repositions the camera each frame (toggle) |
| `0x8001DCF8` | fn_8001DB74                 | `or r0, r0, r3` -> `li r0, 7`: never cull characters |
| `0x806AA4E4` | animateDefence (game.rel)   | `li r0, 2` -> `li r0, 1`                   |
| `0x806AB8B4` | animateOffence (game.rel)   | `li r0, 0` -> `li r0, 1`                   |
| `0x800BD9DC` | fn_800BD8C4                 | `or r3, r4, r3` -> `li r3, 7`              |
| `0x806F7B7C` | loadStadiumObjectVisuals    | `lbz r0, 0x93(r26)` -> `li r0, 3`: never cull stadium hazards |

The three never-cull patches are the same ones `Gecko Codes/Global/Widescreen.c`
applies; both codes write identical words, so enabling both is harmless. The
.ini wrote the two game.rel words unconditionally (into menus.rel too when
the match REL was not resident); the C version only writes a site holding the
expected original, which is the same effect in a match and a no-op otherwise.
