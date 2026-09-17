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
| Restrict Batter Pausing | `Ranked/Ban Batter Pausing.c` | The repo file carries the *Restrict* logic under the *Ban* name. The in-service outright ban is imported separately (see below). |
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
the same job as something already in the repo; **BUG** the original hex has a
defect that the C reproduces on purpose; **GATED** the hex wrote REL memory with
no loaded-REL check (corrupting menus.rel / heap while in menus) and the C adds
`.state = MSSB_GAME` -- the one deliberate behaviour change across the import.

Per-group engine notes: `docs/import_menu.md`, `docs/import_match_gameplay.md`,
`docs/import_match_rules.md`, `docs/import_balance.md`.

### Menu / boot / global (18)

| In-service name | File | Flags |
|---|---|---|
| Ban Characters on CSS | `Menu/Ban Characters on CSS.c` | Hook moved `0x80051B3C` -> `0x80051AA8` (r0 live at the original site; straight-line path, same effect). BUG kept: a player whose cursor is -1 and presses X writes one byte before the table. |
| Boot to Minigames / Practice Mode / Toy Field | `Global/Boot to *.c` | CONFLICT: all hook `0x8063F964`, as do Boot to Progressive mode, `Boot To Main Menu.c`, `Boot To Match.c`. Shared `Include/Rio/BootScene.h`. |
| Boot to Progressive mode | `Global/Boot to Progressive mode.c` | CONFLICT (same site). |
| Hover Over Unused CSS Squares | `Menu/` | JOKE; selecting an unused square likely crashes. |
| Non-captain Bowser is banned | `Menu/` | Hook moved one instruction later for r0. Ruleset code. |
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
| Bobble More | same (ASM x3) | BUGs kept: 25% per site, not the advertised 50%; clobbers cr0 at `0x80665730` (breaks the minigame branch) and r14-r17; rolls on the CPU timebase, so it **desyncs netplay**. |
| Remove Ground Bobbles | same | Normal fields only (not Toy Field). GATED. |
| Every Hit Is A Random Star Hit | same | JOKE. Uses the game's synced RNG, netplay-safe. |
| Every Throw Could Be Anti | same | BALANCE. Overlaps Anti Chem. GATED. |
| Anti Chem Has a 50% of Activating | same | BALANCE. `#define ANTI_CHEM_PERCENT`. |
| Hold Up for Eephus | same (ASM x2) | BUG kept: the `>= 8` test means Z, R or L trigger it too. Clobbers r14. |
| Random Batting Power | same (ASM x2) | JOKE. Inline timebase LCG (`Include/Rio/TimebaseRandom.h`): **desyncs netplay**. |
| Random Star Pitch | same (ASM) | JOKE. Timebase RNG: **desyncs netplay**. |
| Random Star Swing | same (ASM) | JOKE. Timebase RNG. Range 0-12 where Star Pitch is 1-13 (no `+1`). |
| Remove Slice | same | BALANCE. CONFLICT with the Frame 2 variant (first to run wins). Shared `Match/SliceAngles.h`. |
| Remove Slice - Frame 2 Stars | same | BALANCE. Also writes `[2][0][10] = 700`, which looks like a leftover. |
| Star Swing is Always -1 Star v2 | `Star Swing is Always -1 Star.c` | BALANCE. Writes the field the decomp calls `nonCaptain_CaptainStarCost` while the description says captain swings -- one of the two is mislabelled. |
| No Bunting | same | Ruleset. |
| Never Ball Trail | same | Cosmetic. |

### Match rules / stadiums (20)

| In-service name | File (`Match/`) | Flags |
|---|---|---|
| Input Prediction | same | **SUPERSEDES** `Batter Lag Reduction & Positional Correction.c`: identical batting half at the same four sites plus pitch-curve prediction. CONFLICT with it. BUGs kept: pitch prediction always reads port 1's controller (takes the top byte of `teams[0]`); the vertical box-movement reset loads the horizontal change word instead of 0.0f (the Batter Lag conversion *fixed* that one, so the two files currently differ there). Decision needed: keep both, or retire Batter Lag and share one implementation. |
| Dash Glitch Fix | same | Bug fix, per-frame. |
| Reset Stars in 13th Inning | same | Ruleset. |
| Runners Score At Random Bases | same | JOKE. Meaning of the patched float unverified. |
| Superstars Only Boost Batting (Hybrid Star Mode) | same | BALANCE. |
| Unlimited Stars | same | CHEAT/TRAINING. |
| Always 0 outs, 0 balls, and 0 strikes | same | TRAINING. |
| Always X Chemistry Link(s) on Base | `Always X Chemistry Links on Base.c` | TRAINING. `#define` 0-3. |
| More Star Chances | same | BALANCE. Percent is a `#define`. GATED. |
| Remove Baserunning Lockouts | same | OVERLAP with `Rio Built-in/Remove Baserunner Lockout.c`: this overwrites the lockout-length constant the built-in's hook reads, unconditionally, so it overrides the built-in's ball-state condition. GATED. |
| Ban Batter Pausing | `No Batter Pausing.c` | CONFLICT with `Ranked/Ban Batter Pausing.c` (same site `0x806EED5C`; that file holds the in-service *Restrict* logic). |
| Hazardless w/Star Pads on Bowser's Castle | `Hazardless w Star Pads on Bowser's Castle.c` | CONFLICT with `Hazardless Stadiums.c` (same hook `0x80699508`). |
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
| Balance Patch V 1.1 | `Balance Patch.c` | **BALANCE -- prime candidate not to ship.** BUGs kept: Blue Shy Guy "Running 50->60" actually writes `ChargeHitPower` (the stat bar moves, `Speed` does not); an unlogged write sets Boo->Black Shy Guy chemistry 86->90 (looks off by one); the Dry Bones ability change also drops Wall Splat. |
| Balance Patch V 2.0 (April Fools) | `Balance Patch (April Fools).c` | **JOKE -- prime candidate not to ship.** BUGs kept: the "Koopa-Red" stat bars land on Green Koopa (id 12); Petey "Running 3->8" lands on `FieldingStatBar`; the log swaps the two trajectory labels; its Bowser draft hook (`0x80051314`) clobbers r31. CONFLICT with `Menu/Non-captain Bowser is banned.c` (same site) and contains Super Duper Jump. |
| Characters are Big / Small | `Characters Are Big.c`, `Characters Are Small.c` | JOKE. CONFLICT with each other. Shared `CharacterScale.h`. |
| Duplicates & Variants Have Chemistry | same | BALANCE. |
| Everyone Has Anti-Chemistry | same | JOKE/BALANCE. CONFLICT with Everyone Has Chemistry. |
| Everyone Has Chemistry | same | BALANCE. Also carries an unrelated Bowser lefty-contact fix at `0x8065115C`, restored every frame, so it CONFLICTs with Bats Are All Righty. |
| Bats Are All Lefty / Righty | same | BALANCE. CONFLICT with each other. Shared `BatHandedness.h`. |
| Monty 70 Speed, Noki Walljump, Captains Use Non-Captain Stars, All teams are 5-star teams | same | BALANCE. |
| Chemistry Threshold 80, Perfect Slap Buff | same | BALANCE. |
| Super Duper Jump | same | JOKE. Duplicates part of April Fools. Lowers super-jump gravity rather than raising launch speed. Shared `SuperJump.h`. |
| Pitchers Have 1 Stamina / Pitchers never get tired | `Pitchers Have 1 Stamina.c`, `Pitchers Never Get Tired.c` | BALANCE/TRAINING. CONFLICT with each other. Shared `PitcherStamina.h`. |
| Fix Non-red Toad Hitboxes and Bat Reach | `Fix Toad Hitboxes and Bat Reach.c` | The description is backwards for the fielding hitbox: Red Toad is overwritten to match the other Toads. One value is 336 where the other Toads have 350 -- possibly a typo in the original. |

## Decisions still open

1. **Input Prediction vs Batter Lag Reduction** -- same batting half at the same
   sites; retire Batter Lag, or keep both on one shared implementation? They
   currently differ on the vertical box-movement reset bug (fixed in one, kept
   in the other).
2. **Netplay-unsafe codes** (Bobble More, Random Batting Power, Random Star
   Pitch, Random Star Swing): ship as-is, move them onto the game's synced RNG,
   or leave them out?
3. **Known-bug reproductions** -- everything marked BUG above was kept
   bit-for-bit. Each is a candidate for a real fix once it is decided the code
   ships (a fix changes behaviour for every client on the list, so it should be
   a deliberate release, not a side effect of this import).
4. **Balance folder** -- which, if any, belong in a shipped list.
5. `Ranked/Ban Batter Pausing.c` holds the *Restrict* logic; with
   `Match/No Batter Pausing.c` now present the names are misleading.

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
