# Imported in-match gameplay codes (group C_match1)

Engine notes behind the codes imported from Project Rio's hex-only list into
`Gecko Codes/Match/`. All addresses are game.rel (`.text` at 0x8063F094);
function names are where the disassembly lands (decomp `symbols.txt` offsets
drift by 0x18 in this region). The hex-faithful ports are commit 6e95d11; the
"differs from the in-service hex" notes below describe what changed since.

All per-frame data codes here are gated on `MSSB_GAME`; the hex had no gate and
wrote into the REL slot while menus.rel was loaded.

## RandomInt_Game (0x806DDF4C)

`int RandomInt_Game(int max)` returns **0 .. |max|-1** (negated when `max` is
negative), and 0 when `|max| <= 1`. Each call advances the synced state:
`StaticRandomInt1 += StaticRandomInt2 / |max| - (u8)StaticRandomInt2 +
totalFramesAtPlay`, result = `|StaticRandomInt1 % |max||`. Two special cases:
in Practice with an instruction active it always returns 0, and in Minigames
it mixes in libc `rand()`. It uses no FPRs, so it is safe to call from a cgecko
C hook (whose wrapper saves r3-r31 but neither CR nor FPRs).

Because the state is part of the netplay-synced game state, every client rolls
the same number. Calling it from a mod advances the state, which is fine as
long as all clients run the same mods.

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

`calculateBobble` returns early, leaving its initial reset value, when the
ball was already handled, hit a wall, sat on the ground >= 5 frames, in
tutorials, or on a Super Catch.

Codes that touch this:

- **Bobble Always / Disable Bobbles** write `bobble` = 3 / 0 for all nine
  fielders every frame. They overwrite each other and override every other
  bobble code.
- **Remove Ground Bobbles** zeroes `BobbleArray[0][class][0..2]` (normal
  fields only, not Toy Field).
- **Bobble More** overrides the game's three "no bobble" stores
  (`li rX, 0 ; stb rX, 0x258(fielder)`) with `RandomInt_Game(2) ? 3 : 0`:

  | game store | hook | how |
  |---|---|---|
  | 0x8066B9C4 `checkForCatchBallAction` | 0x8066B9BC (`lbz r0, 0x1BC9(r5)`, re-run) | hook sets `bobble`, and nops the store that follows |
  | 0x80665734 `calculateBobble` initial reset | 0x80665718 (`mr r31, r28`, re-run) | same; fielder index is in r28 |
  | 0x80665AE8 `calculateBobble` ground "no bobble" | 0x80665AE8 itself | replaces the store; r0 is reloaded right after |

  The hooks sit where r0 and cr0 are dead. The two stores that cannot be
  hooked directly (r0 live at the first; cr0 live across the second, between
  the minigamesEnabled `cmplwi` and its `beq`) are nopped from inside the hook
  that runs just before them, so the patch is always in place in time.

  Differs from the in-service hex: the hex rolled on the CPU timebase
  (`mftb`), which desyncs netplay; it took `((tb >> 22) & 0xFFFF) * 16 >> 16`
  (0-15) and bobbled only when that was `> 11`, i.e. 4 of 16 = 25%, not the
  advertised 50% (the author most likely meant `> 7`); its `cmpwi` clobbered
  cr0 at 0x80665730 so the minigame fielder-index branch followed the dice;
  and it left r14-r17 modified. All four are fixed.

## Bad-chemistry ("anti") throws

`makeThrowVariables` (contains 0x806EAFF8) computes the chemistry between
thrower and target and compares it with `chemThresholds` (0x807B5C38,
`s16[4]`, stock 90, 20, 90, 90): `chem >= [0]` flags a good-chemistry throw;
`chem < [1]` makes the throw eligible to go wild. For an eligible throw it
interpolates a chance into r31 and rolls `RandomInt_Game(100)` into r3;
`cmpw r3, r31 ; bge <normal>` at 0x806EAFF8 decides. `[2]` is the buddy-toss
threshold.

- **Every Throw Could Be Anti** sets `[0] = 100, [1] = 99`, so every pair
  below 99 chemistry is eligible.
- **Anti Chem Has a 50% of Activating** replaces the compare with
  `cmplwi r3, ANTI_CHEM_PERCENT`.

## Pitch speed reads (Hold Up for Eephus)

`adjustPitchCurveSpeedCursedBall` copies `g_Pitcher.curveBallSpeed` (+0x142)
to `pitchSpeed` (+0x14A) at 0x806B1F30/34, and feeds `fastBallSpeed` (+0x143,
loaded into r5 at 0x806B1E40) through an int-to-float conversion whose high
word sits in r0 until 0x806B1E4C. The code substitutes speed 0x41 while the
pitcher holds Up.

- Curve: C hook at 0x806B1F34 replaces the `stb r0, 0x14A(r31)`.
- Fast: C hook at 0x806B1E58 (`stw r5, 0x1C(r1)`, re-run), the first point
  after the load where r0 is dead; it overrides r5.

"Up" is `g_FieldingLogic.fielderInputs & INPUT_BUTTON_UP` (0x80892898, bit
0x0008). The hex read the same byte, so the stick is presumably folded into
the d-pad bits there; that was not verified in-game.

Differs from the in-service hex: the hex tested `low byte >= 8`, which also
fired on Z / R / L (0x10 / 0x20 / 0x40) without Up, and clobbered r14. The
second curve-speed copy at 0x806B1F24 (taken when +0x161 == 2) was not hooked
by the hex and still is not.

## Randomised stats

Random Batting Power / Star Pitch / Star Swing are C hooks that call
`RandomInt_Game`.

- **Random Batting Power**: `setDefaultInMemBatter` clamps the two power stats
  to 1-100 and stores them to `g_Batter.hitPower_capped[0]` (0x806536D8) and
  `[1]` (0x80653724, through r4 = g_Batter + 1). One C hook at 0x806536D8
  (r0 and cr0 dead) writes both as `RandomInt_Game(160) + 1` and nops the
  second store, where r0 is live.
- **Random Star Pitch**: hook at 0x806ADF8C in `setPitcherStatsToInMemPitcher`
  replaces `lbz r4, 0x34(r5)` (the stat the game then stores to
  `g_Pitcher.captainStarPitch`) with 1-13.
- **Random Star Swing**: hook at 0x806AD0B0 in `setInMemBatterConstants`
  replaces `lbz r8, 0x34(r6)` (stored to `g_Batter.captainStarHitPitch`) with
  1-12 (`CAPTAIN_STAR_TYPE_MARIO..DAISY`). In `calculateHitVariables` a
  `captainStarHitPitch` of 0 means "no captain star swing, use the regular
  one", so 0 is not a star swing.

Differs from the in-service hex: the hex did not call the game's RNG; it ran
one MSVC LCG step (`x * 214013 + 2531011`) on the CPU timebase, which desyncs
netplay and replays, and zeroed r14-r16 (callee-saved). Star Swing's range was
0-12 (scale by 13 with no +1), so 1 in 13 rolls gave no captain swing; it is
now 1-12. Star Pitch keeps the hex's 1-13; whether 13 is a valid star pitch id
was not verified.

Every Hit Is A Random Star Hit is separate: in `calculateHitVariables`
(0x806517F4) it sets `g_Batter.captainStarSwingActivated =
g_Ball.StaticRandomInt1 % 12 + 1`, reading the synced state without advancing it.

## Slices (BattingAngleRanges)

`BattingAngleRanges` (0x807B6394, `s16 [3][2][15][2]`) is indexed
`[inputDirection][charge/star][framesSinceStartOfSwing][lower, upper]`. A 0,0
entry inside the hittable window sends the ball straight up the middle. Stock
values around the window edges:

| dir | charge | f2 | f3 | f9 | f10 |
|---|---|---|---|---|---|
| 0 | 0 | -700,-500 | -500,-350 | 350,550 | 550,700 |
| 0 | 1 | **0,0** | -600,-400 | 350,600 | **0,0** |
| 1 | 0 | -750,-500 | -500,-300 | 500,600 | 600,700 |
| 1 | 1 | **0,0** | -600,-400 | 500,700 | **0,0** |
| 2 | 0 | -700,-600 | -600,-400 | 300,700 | **0,0** |
| 2 | 1 | **0,0** | -550,-300 | 450,650 | **0,0** |

Remove Slice fills all seven holes with a foul angle, including `[2][0][10]`,
the one non-charge hole (the other two directions have a real range there).
Remove Slice - Frame 2 Stars fills only the three `[dir][1][2]` holes.

Differs from the in-service hex: the Frame 2 variant's hex also wrote
`[2][0][10] = 700`. That entry is a frame-10, non-star swing, so it
contradicts the code's own description and was copied over from the full
table; it was removed.

Both codes apply once, keyed on `[0][1][2]` still being 0, so whichever of the
two runs first wins. Do not enable both.

## Star costs

`starPowerCosts` (0x807B76F4, stock 5, 1, 2, 1): `moonShotCost,
captainStarCost, nonCaptain_CaptainStarCost, regularStarCost`. Verified in
`calculateHitVariables`: with `captainStarHitPitch != 0`, 0x80651764 reads
byte +2 when `Team_CaptainRosterLoc != rosterID` and 0x80651790 reads byte +1
when the batter is the team captain. So the decomp's names are right, and
"Star Swing is Always -1 Star" (byte +2 = 1) lowers the cost for captain
characters who are on the team without being its captain; the team captain
already pays 1. The in-service description ("captain star swings cost 1
instead of 2") was the loose one; the `.notes` now say what happens.

## One-instruction patches

- No Bunting: `batterHumanControlled` 0x806532F0 `beq` -> `b`, always skipping
  the bunt-button block.
- Never Ball Trail: `setupBallTrailEffect` 0x806A66B4 -> `blr`.
