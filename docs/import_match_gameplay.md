# Imported in-match gameplay codes (group C_match1)

Engine notes behind the codes imported from Project Rio's hex-only list into
`Gecko Codes/Match/`. All addresses are game.rel (`.text` at 0x8063F094);
function names are where the disassembly lands (decomp `symbols.txt` offsets
drift by 0x18 in this region).

## Bobbles

`InMemFielder.bobble` (+0x258, `g_Fielders[0..8]`) is decided once per pickup
by `calculateBobble` (0x806656F8):

| value | meaning |
|---|---|
| 0 | clean pickup of a ground ball |
| 1 | clean catch of a ball in the air / before its 2nd bounce |
| 2 | ground-ball bobble |
| 3 | air-ball bobble |
| 4 | set elsewhere (chemistry/bad-throw fumble path) |

The roll is `RandomInt_Game(1000) < BobbleArray[toyField][characterClass][kind] * 10`.
`BobbleArray` (0x807B5D14, `u8 [2][4][6]`): `kind` 0-2 are the ground-ball
chances (centre catch, side/running catch, diving/action), 3-5 the same for
air balls. The pitcher's (fielder 0) air-ball chance is doubled.

`calculateBobble` returns early with bobble = 0 when the ball was already
handled, hit a wall, sat on the ground >= 5 frames, in tutorials, or on a
Super Catch.

Codes that touch this:

- **Bobble Always / Disable Bobbles** write `bobble` = 3 / 0 for all nine
  fielders every frame. They overwrite each other and override every other
  bobble code.
- **Remove Ground Bobbles** zeroes `BobbleArray[0][class][0..2]` (normal
  fields only, not Toy Field).
- **Bobble More** hooks the three `li rX, 0 ; stb rX, 0x258(fielder)` "no
  bobble" stores: 0x80665730 (`calculateBobble`'s initial reset), 0x80665AE4
  (its ground-ball "no bobble" branch) and 0x8066B9C0
  (`checkForCatchBallAction`), replacing the 0 with 3 when
  `((timebase >> 22) & 0xFFFF) * 16 >> 16 > 11`.
  - That is 4 in 16 = 25% per site, not the 50% the description claims
    (two sites can roll on one pickup).
  - The roll reads the CPU timebase (`mftb`), not the game's seeded RNG, so it
    is not netplay/replay deterministic.
  - It leaves r14-r17 (callee-saved) modified.
  - At 0x80665730 the game's `cmplwi r0, 0` (minigamesEnabled) sits before the
    hook and its `beq` after it; the blob's own `cmpwi` overwrites cr0, so the
    minigame fielder-index branch is taken on the random roll instead.

## Bad-chemistry ("anti") throws

`makeThrowVariables` (contains 0x806EAFF8) computes the chemistry between
thrower and target and compares it with `chemThresholds` (0x807B5C38,
`s16[4]`): `chem >= [0]` flags a good-chemistry throw; `chem < [1]` makes the
throw eligible to go wild. For an eligible throw it interpolates a chance into
r31 and rolls `RandomInt_Game(100)` into r3; `cmpw r3, r31 ; bge <normal>` at
0x806EAFF8 decides. `[2]` is the buddy-toss threshold used elsewhere.

- **Every Throw Could Be Anti** sets `[0] = 100, [1] = 99`, so every pair
  below 99 chemistry is eligible.
- **Anti Chem Has a 50% of Activating** replaces the compare with
  `cmplwi r3, ANTI_CHEM_PERCENT`.

## Pitch speed reads (Hold Up for Eephus)

`adjustPitchCurveSpeedCursedBall` reads `g_Pitcher.curveBallSpeed` (+0x142,
0x806B1F30, into r0 then `pitchSpeed`) and `fastBallSpeed` (+0x143,
0x806B1E40, into r5 for a float conversion whose high word is already in r0).
The code substitutes 0x41 when the low byte of
`g_FieldingLogic.fielderInputs` (0x80892899) is >= 8. Bit 3 is stick-up, but
the test is a magnitude compare, so Z / R / L held (0x10 / 0x20 / 0x40) also
trigger it.

## Timebase random (UnclePunch idiom)

Random Batting Power / Star Pitch / Star Swing do not call `RandomInt_Game`.
They run one step of the MSVC LCG (`x * 214013 + 2531011`) on the CPU
timebase, take bits 16-31 and scale to the range. Consequences: not
deterministic across netplay peers or replays, and r14-r16 (callee-saved) are
zeroed afterwards. Shared as `Include/Rio/TimebaseRandom.h`.

Sites: `setDefaultInMemBatter` stores of `hitPower_capped[0]` (0x80653724,
0x806536D8; only element 0 is randomised), `setPitcherStatsToInMemPitcher`'s
store of `captainStarPitch` (0x806ADF98, 1-13), `setInMemBatterConstants`'s
store of `captainStarHitPitch` (0x806AD0B8, 0-12; this one has no +1).

Every Hit Is A Random Star Hit is different: in `calculateHitVariables`
(0x806517F4) it sets `g_Batter.captainStarSwingActivated =
g_Ball.StaticRandomInt1 % 12 + 1`, which uses the game's synced RNG state.

## Slices (BattingAngleRanges)

`BattingAngleRanges` (0x807B6394, `s16 [3][2][15][2]`) is indexed
`[inputDirection][charge/star][framesSinceStartOfSwing][lower, upper]`. The
slice entries are `[dir][1][2]` and `[dir][1][10]`, stock value 0,0 (straight
up the middle). The codes set both bounds to a foul angle (+-550..700). Both
variants also set `[2][0][10]` (a non-charged entry) to 700 -- present in the
original hex of both, including the "frame 2 only" one. The codes apply once,
keyed on `[0][1][2]` still being 0, so whichever of the two runs first wins.

## Star costs

`starPowerCosts` (0x807B76F4): `moonShotCost, captainStarCost,
nonCaptain_CaptainStarCost, regularStarCost`. "Star Swing is Always -1 Star"
writes byte +2, which the decomp names `nonCaptain_CaptainStarCost`, while its
description talks about captain star swings.

## One-instruction patches

- No Bunting: `batterHumanControlled` 0x806532F0 `beq` -> `b`, always skipping
  the bunt-button block.
- Never Ball Trail: `setupBallTrailEffect` 0x806A66B4 -> `blr`.
