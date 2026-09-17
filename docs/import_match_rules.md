# Imported match codes (rules, stars, stadium objects): engine notes

Engine facts behind the in-service codes imported into `Gecko Codes/Match/`
from hex. Addresses are game.rel as Rio loads it (`.text` 0x8063F094, `.rodata`
0x807AD4F8, `.data` 0x807B1600). menus.rel ends at 0x807082E4, so every
game.rel `.data` address below is heap memory while the menus are up; the
in-service hex wrote several of them with no REL check. The C versions are all
gated on `MSSB_GAME`.

## Input Prediction

Successor of `Batter Lag Reduction & Positional Correction` (see
`docs/match_codes.md` for the batting half: same four sites `0x80651D48`,
`0x80652360`, `0x80652D00`, `0x80652D9C`, the same two `cmpwi` patches and the
same `hittableFrameInd[0][1]` write, the same claimed words). The two cannot be
enabled together. Differences found while importing:

- **The gate is `22`, not a 16-bit compare.** `2289091C 00000000` is gecko's
  32-bit *if not equal*: the swing pieces apply while the word at
  `g_Batter.batPosition.y` is **non-zero**. The earlier conversion read it as
  "upper halfword == 0", which is the opposite sense. Gecko `if`s only gate the
  per-frame writes and the *installation* of a C2; once installed a hook stays
  until the REL reloads. So here only the per-frame patches test the word and
  the hooks run unconditionally, as the hex blobs do.
- **Box movement, Z axis.** When the previous position does not match, the hex
  resets the previous change from `-4(base)`, which for the Z hook is the X
  change word rather than the 0.0f at `0x802EBF9C`. Kept. The register hygiene
  fixes of the earlier conversion (r8/f12/f13 and a stack slot instead of
  r19/f28/f29 zeroed afterwards and a 2.0f written into `g_Batter + 0x20`) are
  carried over.

Pitching half (new):

- `0x8069E1D4`, `atBatScreen` epilogue (`lwz r0, 0x14(r1)`, reloaded by the
  re-run instruction): stores the curve input seen this frame, from the low
  byte of `g_FieldingLogic.fielderInputs` (`0x80892899`): LEFT if held, else
  the RIGHT bit. Claimed byte `0x802EBF98`.
- `0x806B05C4`, straight after the pitch update's `bl pitchCurve`
  (`0x806AFF88`); the replaced `lis r3` is relocated to `lis r3, 0x8089`. If
  the curve input read from the controller differs from the stored one,
  `pitchCurve()` runs a second time and `g_Pitcher.pitchCurveVeloV1` becomes
  the sum of its value after the first and after the second call (kept in the
  claimed word `0x802EBFB4`): the new input is applied a frame early. r0 and
  f0-f3 are reloaded right after the site.
- Original bug, kept: the controller is indexed with `lbz 0xEC(g_GameLogic)`,
  the **top byte** of the u32 `teams[0]`, which is always 0, so the prediction
  always reads port 1's buttons. (A dead `slwi r6, r6, 4` next to it shows the
  port-times-16 indexing that was intended.)

## Dash Glitch Fix (per frame)

The first `FielderDash` record at `g_FieldingLogic + 0`: while a match is
running (`g_GameLogic.EventTriggers_GameHasStarted == 1`) and
`sprintingState != 0`, a `dashingFielderIndex` that is neither 0xFF nor
`g_FieldingLogic.selectedFielder` (`+0xB0`) is overwritten with the selected
fielder. The header's `fielderDashByPort[4][0x1B]` cannot be right (a 0x1B
stride puts s16 fields on odd addresses); only record 0 is used here.

## Reset Stars in 13th Inning (`0x8069C6E4`, inningChange+0x68)

The site is `stw r0, 0(r31)`, the store of `Inning = r3 + 1` on the
bottom-to-top path only, so it fires once per new inning. r0 is the value, so
the C body does the store itself from r3 and has no `.instruction`. The hex
tests the low byte of the inning. Star counts are restored from
`Static_Stats_Tables.startingChemStars` (`0x803530AF`).

## Runners Score At Random Bases (`0x8069C6E8`, inningChange+0x6C)

Runs on every half-inning change (both paths merge here). It rewrites the
game.rel `.rodata` float at `0x807AE8B0` (`lbl_3_rodata_13B8`, stock 4.0f --
taken to be the base-path distance at which a runner scores; its readers were
not disassembled). The value is built from `g_Ball.StaticRandomInt1`: the low
10 bits become float bits 15..24 on top of `0x3C000000`, i.e. exponent
2^-7..2^-4 with a random mantissa, times 32: a distance in [0.25, 4.0), not
uniform (each power-of-two band is equally likely). The hex clobbered the
non-volatile f30/f31; the C body does not.

## Superstars Only Boost Batting (DOL, transferStatsToInMemRoster)

`0x800427F0..` adds the twelve superstar boosts (table at `0x800E86F0`) to the
in-memory `StatTable`. The seven stores that are nopped: `Speed` `0x80042880`,
`ThrowingArm` `0x8004288C`, `CurveBallSpeed` `0x800428A4`, `FastBallSpeed`
`0x800428B0`, `cursedBall` `0x800428BC`, `Curve` `0x800428C8`, `curveControl`
`0x800428D4`. The contact/power stores and the `BattingStatBar + 2` stay.

## More Star Chances (`0x8069E788`, matchTransitionPrepareNextAB)

`cmpw r3, r0` compares a 0..99 roll (`bl 0x806DDEB8` with r3 = 100) with a
chance byte; below it `IsStarChance` is set. Replaced by `cmpwi r3, 75`.

## Always 0 outs, 0 balls, 0 strikes

`g_Strikes.strikes/balls/outs` are s32; the hex writes only byte 3 of each.

## Remove Baserunning Lockouts (`0x807B625A`)

game.rel `.data` `lbl_3_data_4C54` (`0x807B6254`) is an s16 table
`{30, -20, 3, 30, 2, 20, 4, 6, 1, 4000, 1, 3}`. `running_MainFunction` holds
its address in r29 and reads entry 3 (`lha r0, 6(r29)` at `0x806C9D78`) as the
number of frames after contact before runners take input. This code overwrites
that constant (30 -> 1) permanently. Rio's built-in `Remove Baserunner Lockout`
hooks the read instead and only substitutes 1 while `g_Ball.ballState == 0`.
Same value, so with both on the built-in's condition no longer matters.

## No Batter Pausing (`0x806EED5C`, match_checkForPause)

`lhz r0, 6(r4)` is the batter's new-button read; `li r0, 0` means START is
never seen. `Ranked/Ban Batter Pausing.c` is a C2 at the same address (pause
allowed while standing still); one of the two wins depending on order.

## Stadium object placement tables (`Include/Rio/StadiumObjects.h`)

Both tables are arrays of 0x14-byte records `{f32 x, y, z, rotation; u8 type;
u8[3]}` with y pointing down (negative is up).

- `thwompStaticValues` `0x807C8B14`: the codes touch the first six records'
  `y`. Stock -18.0; `Lock Thwomps Down` writes -5.0; `Hazardless w Star Pads`
  writes +50.0, which buries them.
- `blocks` `0x807CD098` (Peach Garden), first 16 records. `type`: 0 brick,
  1 metal, 2 note, 3 outline. Stock y is -12; 0 is on the ground; the wall code
  uses rows at 0 and -4, x = +-2/6/10/14, z = 30, rotation 0.

Gecko `08` slider writes (`08AAAAAA VVVVVVVV / TNNNIIII DDDDDDDD`) became plain
loops over these records.

## Hazardless w Star Pads on Bowser's Castle (`0x80699508`)

Same hook and the same patches as `Hazardless Stadiums`
(`docs/match_codes.md`), plus the per-frame thwomp burial above. In the hex
the Yoshi / Peach / DK patches are applied on every stadium rather than per
stadium; kept.

## Remove Bowser Castle Screen Shake (`0x80702B34`, thwomp_screenShake)

`li r4, 60` is the shake length stored to `thwompScreenShakeTimeRemaining`;
patched to 0.
