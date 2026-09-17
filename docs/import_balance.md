# Imported balance codes (`Gecko Codes/Balance/`)

Nineteen in-service Rio codes that existed only as hex, brought in as C. This
file holds the engine knowledge they rely on, the full change logs that are too
long for a player-facing `.notes`, and every place where the hex disagrees with
what its author said it does. Behaviour was kept identical to the hex except
where noted under "Deliberate differences".

## Tables these codes edit

### Master stat table: `Static_Stats_Tables.characterStats[CHAR_ID]`

0x8034E9A0, 54 rows of `CharacterStats` (0xA0). `stats` is the 0x3B-byte
`StatTable`; `chemistry` (0x3B..0x70) is one byte per partner, **indexed by the
partner's `CHAR_ID`**. The table is BSS filled at boot (it is not in main.dol's
data), and it is copied into `inMemRoster` when a match starts, so edits must be
in place before the match loads. Stock values used for cross-checking came from
an unmodified savestate.

`Include/Rio/StatEdits.h` expresses byte edits as `STAT(id, field, value)`,
`CHEMISTRY(id, partnerId, value)` and `ABILITIES_LOW/HIGH(id, flags)`.

- `StatTable.FieldingStats` is a big-endian u32 of `FIELDING_ABILITIES` flags.
  The hex codes only ever write one byte of it: offset 0x23 is the low byte
  (wall splat .. magical catch), offset 0x22 is bits 8-15 (tongue catch ..
  super curve). `ABILITIES_LOW/HIGH` take the real flag values and pick the
  byte. A byte write *replaces* every flag in that byte.
- `CaptainStarHitPitch` (0x34) is a `CAPTAIN_STAR_TYPE`; 0 makes a captain fall
  back to `NonCaptainStarSwing/Pitch`. It also selects the captain background in
  the draft, which is why "Captains Use Non-Captain Stars" shows Mario's.
  Writing a non-zero value on a non-captain (Balance Patch gives Yellow Shy Guy
  `CAPTAIN_STAR_TYPE_WARIO`) gives that character the captain star.
- `ChemistryTable`'s member names `RedKoopa` (0x0C) and `GreenKoopa` (0x2A) are
  swapped relative to `CHAR_ID` (12 = `CHAR_ID_KOOPA_GREEN`, 42 =
  `CHAR_ID_KOOPA_RED`). Stock stats and the V 1.1 log agree with `CHAR_ID`.
  The codes index chemistry by `CHAR_ID` and never use those member names.
- `Static_Stats_Tables.startingChemStars[2]` (0x803530AF) is the per-team star
  count the match reads; "All Teams Are 5-Star Teams" holds both at 5.
- `Static_Stats_Tables.charOnCharacterGridSelected[CHAR_ID]` (0x803530F7) is the
  draft "taken" table. Holding an entry at 1 makes the character unpickable.

### Pitcher stamina

Not in the headers yet. `Static_Stats_Tables + 0x4C28` (0x803535C8) is an array
of 18 (`[2 teams][9 players]`) 0x1E-byte pitcher stat records, directly before
`batterStats`. The s16 at +0x10 is the stamina counter: `initializeStats`
(0x806BA39C) loads a constant 10 (`lha r3` then `clrlwi r7, r3, 16` at
0x806BA3DC) and stores it with `sth r7, 0x10(r5)`. Runs allowed and star pitches
count it down.

- "Pitchers Have 1 Stamina" replaces the `clrlwi` with `li r7, 4`.
- "Pitchers Never Get Tired" holds the low byte (+0x11) at 10 every frame. The
  hex writes only that byte, and so does the C.

### Character size: `charSizeMultipliers[CHAR_ID][2]` (0x800E8300, main.dol)

Column 0 is the model scale; the hex never touches column 1. Big and Small are
the stock sizes times 2.5 and 0.25. `CharacterScale.h` keeps the stock sizes as
decimal literals and folds `stock * CHARACTER_SCALE` at compile time **in
double, then rounds to f32** -- folding in float gives 0x403CCCCC instead of the
in-service 0x403CCCCD for 1.18 * 2.5. The per-frame copy goes through `u32`
pointers so the hook never touches a floating-point register.

### Jumps: `jumpArray[hasSuperJump][4]` (0x807B5E84, game.rel)

`{ launch speed, gravity per frame, horizontal drift, frames }` (see
`fielder.c`: `jumpVelocity.y -= jumpArray[hasSuperJump][1]`). "Super Duper Jump"
sets `[1][1]` from 0.02 to 0.005, so the jump is higher because it falls slower,
not because it launches faster.

### Other game.rel data

- `chemThresholds[4]` (s16, 0x807B5C38), stock `{90, 20, 90, 90}`. Entry 0 is
  the good-chemistry threshold.
- `BallHitArray[2][5]` of `{ s16 powerLower, powerUpper, addedGravity }`
  (0x807B6D74), indexed `[BAT_CONTACT_TYPE][HIT_CONTACT_TYPE]`. Perfect Slap
  Buff sets `[SLAP][PERFECT]` from (145, 150, -110) to (165, 180, -75).
- `FielderHitboxConsts[55]` (8 x s16, 0x807B8FB4) and `BatterHitbox[54]`
  (`BatterReachStruct`, 0x807B84F8).

### Lefty bat mirroring (game.rel, unnamed function around 0x80651100)

`beq +8` at 0x80651158 skips `fneg f9, f9` at 0x8065115C for right-handed
batters. NOPing the `beq` mirrors everyone (all lefty); NOPing the `fneg`
mirrors no one (all righty). `BatHandedness.h` names the two sites.

## Deliberate differences from the hex

The hex writes into game.rel (`04`/`02` lines at 0x8065xxxx-0x807Bxxxx) are
unconditional, so they also land while menus.rel occupies the same addresses;
0x8065115C in menus.rel is an `addi r1, r1, 0x30` function epilogue. Following
the repo convention, REL writes here carry `.state = MSSB_GAME`, and instruction
patches use `PatchInstruction_Conditional` against the stock instruction. This
affects Bats Are All Lefty/Righty, Pitchers Have 1 Stamina, Super Duper Jump,
Chemistry Threshold 80, Perfect Slap Buff, the jump part of the April Fools
patch, and the Bowser part of Everyone Has Chemistry. "All Teams Are 5-Star
Teams" had an explicit "is game.rel loaded" check, which became `.state`.

## Where the hex disagrees with its own description

Behaviour was kept; these are for the maintainer.

Balance Patch (V 1.1)
- Blue Shy Guy: the log says Running 50 -> 60. The write lands on
  `ChargeHitPower` (50 -> 60). `Speed` stays 50, while `RunningStatBar` is
  raised 5 -> 6 as if speed had changed.
- Boo: an extra write sets Boo -> Black Shy Guy chemistry 86 -> 90. Not in the
  log and not reciprocated; it sits one byte before the four Dry Bones entries,
  so it looks like an off-by-one.
- Dry Bones (gray/red/blue) "Sliding Catch -> Magical Catch": the low ability
  byte goes 0x09 -> 0x80, which also removes Wall Splat.

Balance Patch (April Fools) (V 2.0)
- "Koopa-Red" stat bars 5/4/4/5 -> 9: the writes land on `CHAR_ID` 12, which is
  Green Koopa.
- Petey "In-game Running 3 -> 8": lands on `FieldingStatBar` (3 -> 8). Petey's
  `RunningStatBar` is 1 and is untouched.
- The log calls 0x2D "Vertical Trajectory (push)" and 0x2E "Horizontal
  Trajectory (mid)". The headers (and the V 1.1 log) have them the other way:
  0x2D `HitTrajectoryPushPull`, 0x2E `HitTrajectoryHighLow`.
- "Non-Captain Bowser Banned" is the `BanNonCaptainBowser` hook in
  `css_initValues` (0x80051314, replacing `stb r0, 0x33(r5)` where r5 =
  `charSelectStruct` and r0 = -1). It stores 0 instead of -1 to
  `charSelectStruct + 0x33`, marks Bowser taken in
  `charOnCharacterGridSelected`, and restores r0 = -1 for the stores that
  follow. It overwrites r31, which the function restores from the stack before
  returning. The per-frame code also holds `charSelectStruct + 0x40`
  (0x803C6068) at 0. What those two roster bytes mean was not worked out.

Everyone Has Chemistry
- Carries an unrelated second feature: while Bowser bats, the lefty `fneg` is
  NOPed (righty contact zone), otherwise it is restored. It restores the
  instruction every frame, so it fights "Bats Are All Righty" if both are on.

Fix Toad Hitboxes and Bat Reach
- The in-service description says the other Toads are matched to Red Toad. For
  bat reach that is true (Blue/Yellow/Green/Purple get Red's first four
  `BatterReachStruct` floats). For the fielding hitbox it is the reverse: Red
  Toad's `FielderHitboxConsts` entry is overwritten with the other Toads'
  values -- except the sixth value, written as 336 (0x150) where the other
  Toads have 350 (0x15E). Possibly a typo in the original.
- The code applies once per game.rel load, keyed on Red Toad's first hitbox
  value still being the stock 70.

## Full change logs

### Balance Patch (V 1.1)

- Yellow Toad: + Magical Catch. Green Toad: + Super Catch.
- Shy Guy: charge power 55 -> 70. Blue: see above. Yellow: star swing -> Phony
  Ball. Green: curve 60 -> 70. Black: throwing arm 50 -> 60. All five: King Boo
  chemistry 90 (both directions).
- Red Koopa: charge power 60 -> 85. Green Koopa: curve 50 -> 70, arm 50 -> 60.
- Peach: arm 40 -> 70. Waluigi: curve 70 -> 100, star swing -> Phony Ball.
- Red Paratroopa: bunt 20 -> 60, vertical trajectory low -> mid.
- Dry Bones: gray/red/blue Sliding Catch -> Magical Catch; red charge power
  60 -> 70; all four Boo chemistry 90 (both directions).
- Baby Mario: speed 70 -> 80, arm 30 -> 40, + Sliding Catch. Baby Luigi: star
  swing grounder -> line drive.
- Paragoomba: speed 70 -> 90. Bowser Jr.: speed 40 -> 50.
- Monty: curveball speed 120 -> 140, fastball 155 -> 165.
- Blue and Red Noki: + Ball Dash.
- Piantas: blue charge 65 -> 75 and speed 20 -> 30; red charge 70 -> 80; yellow
  charge 65 -> 75 and fastball 165 -> 190.
- Menu stat bars adjusted to match.

### Balance Patch (April Fools) (V 2.0)

Gameplay: Super Duper Jump; non-captain Bowser banned; Shy Guys removed from
the draft; Peach has 99 chemistry with everyone, in both directions.

Characters: Mario/Luigi chemistry with each other 99 -> 5. DK nice/perfect spot
30/15 -> 99/99, charge 80 -> 85. Diddy curveball speed 115 -> 50, fastball
155 -> 200, curve 80 -> 101. Peach trajectory (0x2E) 2 -> 0. Yoshi and Birdo
swap captain stars. Baby Mario arm 30 -> 80, Baby Luigi arm 30 -> 70, both
weight 0 -> 9 and Mario/Luigi chemistry 90. Bowser charge 95 -> 99, trajectory
(0x2D) 1 -> 2, speed 10 -> 40. Wario Body Check + Ball Dash, trajectory (0x2D)
0 -> 2, captain star -> Waluigi's. Waluigi curve 70 -> 35, no captain star.
Green Koopa stat bars all 9. All five Toads: chemistry with each other 99 -> 90.
Boo curve 90 -> 100, DK chemistry 68 -> 90. Toadette arm 40 -> 20. Monty gains
Super Curve, Body Check, Ball Dash, Super Jump, Quick Throw, Laser. Bowser Jr.
Super Curve + Body Check, slap 55 -> 56, speed 40 -> 70. Blue Pianta Super Jump
only, speed 20 -> 40. Red Pianta spots 255/255, charge 70 -> 99, batting bar 9,
Mario chemistry 86 -> 90. Yellow Pianta curveball speed 125 -> 105, fastball
165 -> 205, curve 40 -> 205, speed 10 -> 30. Hammer Bro slap 10 -> 5.
Toadsworth Laser only, speed 40 -> 80, arm 30 -> 60. Magikoopas + Super Jump.
King Boo curve 60 -> 99. Petey nice spot 30 -> 40, perfect spot 10 -> 0, speed
10 -> 80, fielding bar 3 -> 8. Dixie `cursedBall` and `curveControl` 50 -> 99.
Goomba arm 30 -> 5. Green Paratroopa curve 60 -> 65. Green Dry Bones
`cursedBall` 90 -> 10. Fire Bro Super Curve + Body Check, slap 5 -> 45.
Boomerang Bro Body Check only, slap 20 -> 5.

## Upstream candidates for the decomp

- `ChemistryTable.RedKoopa` / `GreenKoopa` are swapped.
- Pitcher stat records: `Static_Stats_Tables + 0x4C28`, `[2][9]`, size 0x1E,
  s16 stamina at +0x10.
- `jumpArray`, `chemThresholds`, `BallHitArray` / `hitball_power_range`, and
  `FielderHitboxConsts` are declared only inside decomp `.c` files, so the sync
  gives this repo just their `_ADDR`; each code re-declares the type locally.
- The lefty mirror function around 0x80651100 (game.rel) has no name.
- `charSelectStruct` (0x803C6028) is an untyped `s8[0x94]`; +0x28..0x4B are
  initialised to -1 by `css_initValues`.
