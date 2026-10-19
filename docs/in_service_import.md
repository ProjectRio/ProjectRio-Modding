# In-service gecko code import

Tracks the import of Project Rio's in-service gecko code list (102 codes,
snapshot taken 2026-09-17) into this repo as C. Branch:
`import-in-service-codes`.

Rule used: a code already in the repo is NOT re-imported, even when the
in-service hex differs -- the repo copy is treated as current and the
in-service one as possibly deprecated. "Already in the repo" is decided by what
the code does and which sites it touches, not by its name.

## Already in the repo (28, not imported)

| In-service name | Repo file | Note |
|---|---|---|
| All Duplicate Characters | `Menu/Duplicate Characters.c` | |
| Anti Quick Pitch | `Ranked/Anti Quick Pitch.c` | |
| Batter Can Hold Z to show Easy Batting | `Match/Hold Z To Show Easy Batting.c` | |
| Batter Lag Reduction + Positional Correction v1.2 | `Match/Batter Lag Reduction & Positional Correction.c` | |
| Boot to Main Menu | `Global/Boot To Main Menu.c` | |
| CPU vs CPU | `Match/CPU vs CPU.c` | |
| Custom Music | `RioModPack/Custom Music.c` | |
| Default Competitive Rules | `Ranked/Default Competitive Rules.c` | |
| Dictionary Replaces Menu Music | `Menu/Dictionary Replaces Menu Music.c` | |
| Dictionary Replaces Records | `Menu/Dictionary Replaces Records.c` | |
| Disable Replays | `Match/Disable Replays.c` | |
| Drop Spot On Ball Shadow | `Match/Highlight Ball Shadow.c` | Same two patch sites (`0x806A85B8`/`0x806A85D4`). In-service applies them unconditionally (drop spot moves onto the shadow); the repo version applies them only when drop spots are off. Treated as the same mod. |
| Enable Controller Rumble | `Rio Built-in/Enable Controller Rumble.c` | |
| Hazardless Stadiums | `Match/Hazardless Stadiums.c` | |
| Instant Randoms | `Global/Instant Randoms.c` | |
| Manual Fielder Select | `Ranked/Manual Fielder Select.c` | In-service is the older Y/X/R/Z scheme with a 15-frame lockout; the repo has v5 (R/Z). Old version intentionally not imported. |
| Night Time Mario Stadium | `Menu/Nighttime Mario Stadium.c` | |
| No Captain Required | `Menu/No Captains.c` | |
| Pitch Clock v2.1 | `Ranked/Pitch Clock.c` | |
| Remove Dingus Bunting | `Ranked/Remove Dingus Bunt.c` | |
| Restrict Batter Pausing | `Ranked/Restrict Batter Pausing.c` | Was `Ranked/Ban Batter Pausing.c`; renamed to match its behaviour (pause only while standing still). |
| Skip Memory Card Check on Main Menu | `Rio Built-in/Skip Memory Card Check on Main Menu.c` | |
| Smash C-Stick | `Global/Smash C Stick.c` | |
| Teams Exhibition | `Global/Teams Exhibition.c` | |
| Toy Field Exhibition Game | `Menu/Toy Field Exhibition.c` | |
| Unlimited Extra Innings | `Ranked/Unlimited Extra Innings.c` | |
| Unlock Everything | `Ranked/Unlock Everything.c` | |
| Widescreen | `Global/Widescreen.c` | |

## Imported (74)

Flags: **BALANCE** changes game balance / character stats; **JOKE** not meant
for real play; **CHEAT/TRAINING** practice aid; **CONFLICT** shares a hook site
or data with another code (only one may be on); **SUPERSEDES / OVERLAP** does
the same job as something already in the repo; **FIXED** the original hex had a
defect that the C corrects (the hex-faithful version is commit 6e95d11); **GATED** the hex wrote REL memory with
no loaded-REL check (corrupting menus.rel / heap while in menus) and the C adds
`.state = MSSB_GAME` -- the one deliberate behaviour change across the import.

Per-group engine notes: `docs/import_menu.md`, `docs/import_match_gameplay.md`,
`docs/import_match_rules.md`, `docs/import_balance.md`.

### Menu / boot / global (18)

| In-service name | File | Flags |
|---|---|---|
| Ban Characters on CSS | `Menu/Ban Characters on CSS.c` | Hook moved `0x80051B3C` -> `0x80051AA8` (r0 live at the original site; straight-line path, same effect). FIXED: a player with no cursor on the grid (-1) pressing X wrote outside the table; now bounds-checked. Shares `Include/Rio/CssSquares.h`. |
| Boot to Minigames / Practice Mode / Toy Field | `Global/Boot to *.c` | CONFLICT: all hook `0x8063F964`, as do Boot to Progressive mode, `Boot To Main Menu.c`, `Boot To Match.c`. Shared `Include/Rio/BootScene.h`. |
| Boot to Progressive mode | `Global/Boot to Progressive mode.c` | CONFLICT (same site). |
| Hover Over Unused CSS Squares | `Menu/` | JOKE; selecting an unused square likely crashes. |
| Non-captain Bowser is banned | `Menu/` | Hook moved one instruction later for r0. Ruleset code. CONFLICT with Balance Patch (April Fools), which now uses the same hook. |
| Default 1 / 3 / 7 / 9 Innings | `Menu/Default N Inning(s).c` | CONFLICT with each other and with `Ranked/Default Competitive Rules.c` (writes innings too). Shared `Include/Rio/GameSettingsDefaults.h`. |
| Change Practice Mode Fielders | `Match/` | GATED. |
| Infinite Mini-Game Turns | `Match/` | CHEAT. |
| Max/Infinite Coins | `Global/Max Infinite Coins.c` | CHEAT. Renamed ("/" is not legal in a filename). |
| Disable Music | `Global/` | Hex patched `0x806CCCB0` unconditionally (hits a `stw r31` in menus.rel); now conditional on the game.rel instruction. |
| Never Cull Characters, Never Cull Stage Hazards | `Match/` | OVERLAP: the same patches already live inside `Global/Widescreen.c` and `Rio Built-in/Allow Freecam From External Programs.c`. Harmless together (idempotent). |
| Disable Haze on Every Stadium | `Match/` | GATED. Meaning of the haze bytes inferred, not confirmed. |

### Match gameplay (17)

| In-service name | File (`Match/`) | Flags |
|---|---|---|
| Auto Dingers - hold Z | same | CHEAT. GATED. |
| Bobble Always | same | JOKE. CONFLICT with Disable Bobbles. GATED. |
| Disable Bobbles | same | CONFLICT with Bobble Always. GATED. |
| Bobble More | same (C x3) | FIXED: now the advertised 50% (the hex scaled to 0-15 and tested `> 11`, i.e. 25%); rolls `RandomInt_Game` (synced, netplay-safe) instead of the CPU timebase; no longer clobbers cr0 at `0x80665730` (minigame branch works) or r14-r17. Hooks moved to r0-dead sites; the displaced stores are nop'd. |
| Remove Ground Bobbles | same | Normal fields only (not Toy Field). GATED. |
| Every Hit Is A Random Star Hit | same | JOKE. Uses the game's synced RNG, netplay-safe. |
| Every Throw Could Be Anti | same | BALANCE. Overlaps Anti Chem. GATED. |
| Anti Chem Has a 50% of Activating | same | BALANCE. `#define ANTI_CHEM_PERCENT`. |
| Hold Up for Eephus | same (C x2) | FIXED: tests the Up bit only (the hex's `>= 8` also fired on Z, R, L); no r14 clobber. Unverified: whether `fielderInputs` folds the control stick into the Up bit (the hex read the same byte). |
| Random Batting Power | same (C) | JOKE. FIXED: synced RNG (`RandomInt_Game`), no register clobbers. |
| Random Star Pitch | same (C) | JOKE. FIXED: synced RNG. Range kept at 1-13 (whether 13 is a valid star pitch id is unverified). |
| Random Star Swing | same (C) | JOKE. FIXED: synced RNG; range is now 1-12 -- 0 means "no captain swing", so the hex's 0-12 sometimes did nothing. |
| Remove Slice | same | BALANCE. CONFLICT with the Frame 2 variant (first to run wins). Shared `Match/SliceAngles.h`. |
| Remove Slice - Frame 2 Stars | same | BALANCE. FIXED: dropped the `[2][0][10] = 700` write (a non-charge frame-10 entry that contradicts the code's own description; it remains in plain Remove Slice). |
| Star Swing is Always -1 Star v2 | `Star Swing is Always -1 Star.c` | BALANCE. Settled by disassembly: the decomp name is right (`+2` = a captain character who is NOT the team captain; stock costs 5,1,2,1). The in-service description was loose; `.notes` corrected. |
| No Bunting | same | Ruleset. |
| Never Ball Trail | same | Cosmetic. |

### Match rules / stadiums (20)

| In-service name | File (`Match/`) | Flags |
|---|---|---|
| Input Prediction | same | Shares `Match/BattingPrediction.h` with `Batter Lag Reduction & Positional Correction.c` (identical batting half); adds pitch-curve prediction. CONFLICT with it (same sites) -- both kept, each has its use. FIXED: pitch prediction read port 1's controller for everyone (top byte of `teams[0]`), now `teams[teamFielding]` and skipped for a CPU pitcher; the vertical box-movement reset loaded the horizontal change word instead of 0.0f. |
| Dash Glitch Fix | same | Bug fix, per-frame. |
| Reset Stars in 13th Inning | same | Ruleset. FIXED: compares the full inning value, not its low byte. |
| Runners Score At Random Bases | same | JOKE. Meaning of the patched float unverified. |
| Superstars Only Boost Batting (Hybrid Star Mode) | same | BALANCE. |
| Unlimited Stars | same | CHEAT/TRAINING. |
| Always 0 outs, 0 balls, and 0 strikes | same | TRAINING. FIXED: clears the whole s32 counters, not just their low bytes. |
| Always X Chemistry Link(s) on Base | `Always X Chemistry Links on Base.c` | TRAINING. `#define` 0-3. |
| More Star Chances | same | BALANCE. Percent is a `#define`. GATED. |
| Remove Baserunning Lockouts | same | OVERLAP with `Rio Built-in/Remove Baserunner Lockout.c`: this overwrites the lockout-length constant the built-in's hook reads, unconditionally, so it overrides the built-in's ball-state condition. GATED. |
| Ban Batter Pausing | `Match/Ban Batter Pausing.c` | CONFLICT with `Ranked/Restrict Batter Pausing.c` (same site `0x806EED5C`). |
| Hazardless w/Star Pads on Bowser's Castle | `Hazardless w Star Pads on Bowser's Castle.c` | CONFLICT with `Hazardless Stadiums.c` (same hook `0x80699508`). FIXED: per-stadium patches apply only on their own stadium; thwomps are buried only on Bowser's Castle. |
| Lock Thwomps Down | same | CONFLICT with the Hazardless variant above (same thwomp fields). Shared `Include/Rio/StadiumObjects.h`. GATED. |
| Peach Blocks All Note Blocks / Brick / Metal / Outlines | same | CONFLICT with each other. GATED. |
| Peach Blocks On Ground, Peach Block Wall | same | CONFLICT with each other (block positions). GATED. |
| Remove Bowser Castle Screen Shake | same | GATED. |

### Balance / character data (19) -- `Gecko Codes/Balance/`

Kept in their own folder so the whole group is easy to leave out of a list.
All table writes go through `Include/Rio/StatEdits.h` as named character +
field rows. The stock table is boot-time BSS (not in main.dol); the change-log
"from" values were checked against it from an unmodified savestate. All GATED.

| In-service name | File | Flags |
|---|---|---|
| Balance Patch V 1.1 | `Balance Patch.c` | **BALANCE -- prime candidate not to ship.** FIXED to match its change log: Blue Shy Guy "Running 50->60" wrote `ChargeHitPower`, now `Speed`; a stray unlogged Boo->Black Shy Guy chemistry write is dropped (every logged link was already present); the Dry Bones change no longer drops Wall Splat. |
| Balance Patch V 2.0 (April Fools) | `Balance Patch (April Fools).c` | **JOKE -- prime candidate not to ship.** FIXED: "Koopa-Red" bars now land on Red Koopa (id 42), Petey "Running" on `RunningStatBar`, and the r31-clobbering Bowser hook is the shared C one (`Include/Rio/CssSquares.h`). AMBIGUOUS: the log's "from" values for both (5/4/4/5 and 3) are exactly Green Koopa's bars and Petey's *fielding* bar, so the author may have meant the hex as written -- see `docs/import_balance.md`. CONFLICT with `Menu/Non-captain Bowser is banned.c` (same hook); contains Super Duper Jump. |
| Characters are Big / Small | `Characters Are Big.c`, `Characters Are Small.c` | JOKE. CONFLICT with each other. Shared `CharacterScale.h`. |
| Duplicates & Variants Have Chemistry | same | BALANCE. |
| Everyone Has Anti-Chemistry | same | JOKE/BALANCE. CONFLICT with Everyone Has Chemistry. |
| Everyone Has Chemistry | same | BALANCE. Also carries an unrelated Bowser lefty-contact fix at `0x8065115C`, restored every frame, so it CONFLICTs with Bats Are All Righty. |
| Bats Are All Lefty / Righty | same | BALANCE. CONFLICT with each other. Shared `BatHandedness.h`. |
| Monty 70 Speed, Noki Walljump, Captains Use Non-Captain Stars, All teams are 5-star teams | same | BALANCE. |
| Chemistry Threshold 80, Perfect Slap Buff | same | BALANCE. |
| Super Duper Jump | same | JOKE. Duplicates part of April Fools. Lowers super-jump gravity rather than raising launch speed. Shared `SuperJump.h`. |
| Pitchers Have 1 Stamina / Pitchers never get tired | `Pitchers Have 1 Stamina.c`, `Pitchers Never Get Tired.c` | BALANCE/TRAINING. CONFLICT with each other. Shared `PitcherStamina.h`. |
| Fix Non-red Toad Hitboxes and Bat Reach | `Fix Toad Hitboxes and Bat Reach.c` | FIXED: the one 336 is now 350 like the other four Toads; `.notes` say which way each change goes (Red Toad gets the others' larger fielding hitbox, the others get Red Toad's bat reach). |

## Decisions taken (2026-09-17)

1. Input Prediction and Batter Lag Reduction are both kept and share one
   implementation (`Match/BattingPrediction.h`).
2. The four timebase-RNG codes were ported to the game's synced RNG
   (`RandomInt_Game`, 0..max-1, advances the shared seed), so they no longer
   desync netplay.
3. Defects found in the originals are fixed, not reproduced. The hex-faithful
   import is preserved as commit 6e95d11 for comparison.
4. `Gecko Codes/Balance/` stays for now; each code is to be reviewed
   individually and none may survive.
5. File names follow behaviour: `Ranked/Restrict Batter Pausing.c` (pause only
   while standing still) and `Match/Ban Batter Pausing.c` (never).

Still unverified: whether 13 is a valid star pitch id; whether
`fielderInputs` folds the stick into the Up bit (Eephus); the April Fools
Koopa/Petey intent; the meaning of the float at `0x807AE8B0` (Runners Score At
Random Bases) and of the haze bytes. None of the import has been run in Dolphin.

## Decomp upstream candidates surfaced by the import

`charSelectStruct` 0x803C6028 (+0x18 `s16[4]` cursor square, +0x20 `s16[4]`
hovered char, +0x28 `u8[36]` square owner, 0xFF = free); `gameSettings`
0x803C5F04 (+0x3E innings index, +0x3F mercy, +0x48/+0x4C drop spots);
`AtBat_ButtonInput1` 0x803C77B8 (0x20-stride records: +0 held, +2 new presses);
`starPowerCosts` 0x807B76F4 (bare `extern`, no address); `BobbleArray`
`u8[2][4][6]`, `chemThresholds` `s16[4]`, `BattingAngleRanges`
`s16[3][2][15][2]`, `jumpArray`, `BallHitArray`, `FielderHitboxConsts` (types
exist only inside decomp `.c` files, so only `_ADDR` macros sync);
`thwompStaticValues` 0x807C8B14 and `blocks` 0x807CD098 (0x14-byte
`{x, y, z, rotation, type}` records); pitcher stat records 0x803535C8
`[2][9]` x 0x1E with s16 stamina at +0x10; `ChemistryTable.RedKoopa` /
`GreenKoopa` are swapped relative to `CHAR_ID`; `g_Controls` is
`InputStruct[4]`, not a pointer; `fielderDashByPort[4][0x1B]` cannot be right
(odd stride under s16 fields); 0x807B6254 s16 table whose entry 3 is the
baserunner lockout length.
