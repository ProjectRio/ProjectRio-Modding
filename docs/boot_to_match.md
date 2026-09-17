# Booting straight into a match (and straight to the menu)

Reference for the three boot codes. Used by:
`Gecko Codes/Global/Boot To Match.c` (predetermined match, patched by the Rio
client), `Gecko Codes/Global/Instant Randoms.c` (random match) and
`Gecko Codes/Global/Boot To Main Menu.c`. The loader mechanics they lean on are
in `rel_loader.md`.

## The force-swap sequence (Instant Randoms)

Runs once, on the first frame `rel == 4`, and flips `rel` to 5 so it never
re-enters (returning to the menu re-boots). The order is `loadDemoMatch`'s
(`0x80642054`):

1. `inningSetting.rel = 5; trigger_rel_change = 1;` then `sndFXStartEx(0x1bb)`
   (the Rio bat sound) as an audible "boot started".
2. Register who is playing, which is what the captain-select load and "P2
   press A to join" would do: `g_d_GameSettings.p2_CPU_match_code =
   P2_CPU_CODE_2_PLAYER_GAME`; `Static_Stats_Tables.playerNumberByPort[0] = 0`,
   `[1] = 1`; `portsActiveInMatch = {0, 0, 0xFF, 0xFF}` (0 = active);
   `player2Ind = 1`; `g_MatchInfo.player2Ind2 = 1`.
3. Captains: two draws with the game's own RNG (`randRange_FUN_80042bf0(high,
   low)`) over the 12-cell captain-select grid `mapCaptainCursorPositionToCharID`
   (Mario, Luigi, Peach, Daisy, Yoshi, Birdo, Wario, Waluigi, DK, Diddy,
   Bowser, Bowser Jr.). The real screen stops P2 taking P1's captain; the code
   nudges to the next cell rather than re-rolling (a reroll loop could never
   terminate, and a hang is worse than a hair of bias).
4. Grid bookkeeping the chain expects: clear `charOnCharacterGridSelected[54]`,
   mark the two captains.
5. Draft: `cursorPositions.roster.rosterCharID[team][slot]` (the
   `structCharSelect` at `0x803C6726` that `copyInfoToInMemRoster` reads; slot 0
   = captain) and `rosterSpotFilledInd = 1`. Duplicates allowed over all 54 ids.
6. Conversion chain, once: `copyInfoToInMemRoster`, `teamLogoDetermination(0/1)`,
   `unsure_FillRosterPositions(0/1)`, `characterSelectScreen(0/1)`,
   `setCaptainLocInRoster`.
7. Stadium: `PatchInstruction_Conditional(0x80067220, 0x38800005, 0x38800006)`
   lets `selectRandomStadium()` pick Toy Field too.
8. Settings: `inningSetting.inningCount` (`0x800E8754`) is the actual inning
   count, not a cursor index. `g_d_GameSettings.home_AwaySetting` = who bats
   first (0 = P1 away). `inningSetting.starSkillsSetting` = the match-wide
   star-swing toggle.
9. Superstars: the team-management menu stars a character by parking
   `aiPosSwapInputs.teamManagement_cursorPos[team]` on its slot (1-indexed) and
   calling `transferStatsToInMemRoster(team)`. The boot drives that primitive
   directly for slots 1..9, then parks the cursor at 0 as the menu does. This
   must come after the roster chain so nothing overwrites it. (The Auto-Superstar
   menu walk at `0x8005A4F4` does not fire in a force-swap boot.)
10. `setPortOfEachPlayer()` derives `g_d_GameSettings.playerPorts`
    (`0x800E874C/D`) from `playerNumberByPort`; with `p2_CPU_match_code != 0` it
    takes P2's port from `playerNumberByPort[1]`.

Other symbols: `batsFirstSetting 0x800E870A`; `Static_MSSB_Data` via
`Static_Stats_Tables 0x8034E9A0`.

Toy Field bunting: `ToyFieldBanBunting` (hook `0x80009404`, game state) writes
`b +0x34` at `0x806532F0` when `StadiumID == STADIUM_ID_TOY_FIELD` to branch
over the bunt path.

### Duplicate characters in a booted match

Two C2 hooks fix the model bind when a roster holds the same character twice:
see `duplicate_characters.md` ("dup-load model bind"). Both hooks *replace*
the instruction they overwrite (no `.instruction`).

### Bundled codes

Instant Randoms `#include`s `Menu/Duplicate Characters.c` (its draft-screen
patches are inert because the boot never visits the draft; the chemistry
writer is what matters) and `Rio Built-in/Game ID.c` (temporary, until the Rio
client ships the REL-link Game ID code: the client's built-in version only
rolls an id on the menu's Start Game button, which this boot never presses).

## Boot To Match: the patchable payload

The Rio client (`MSB_GenerateQuickMatchSetupGeckoCode.cpp`) regenerates this
gecko code from a HUD file: it scans the compiled code for the magic `'RIOB'`
(`0x52494F42`) and overwrites the spec/config bytes that follow. Consequences:

- any layout change to `BootMatchSpec` / `BootStateConfig` / `BootMatchPayload`
  must be mirrored in the client's payload offsets, and the client's embedded
  template refreshed from the newly compiled output;
- every field must be read at runtime. `OPAQUE_PTR(p)` (`__asm__("" : "+r"(p))`)
  launders the pointer so GCC forgets it points at const data. Without it, -O1
  constant-folds every scalar into immediates (equal constants even share one
  register: inning 3 / score_home 3 / balls 3 became a single `li`) and the
  client's byte patching silently stops applying.

Payload layout: `magic` at 0, `spec` at 4 (85 bytes + 1 pad), `config` at 90.

### BootMatchSpec

Rosters and per-player arrays are in the game's native draft-slot order
(matching the HUD's "Roster N" indexing), paired with `positionSwap`.

| field              | meaning                                                  |
| ------------------ | -------------------------------------------------------- |
| captain[2]         | charID of each team's captain                            |
| roster[2][9]       | charIDs                                                  |
| battingHand/fieldingHand[2][9] | 0 = right, 1 = left                          |
| superstar[2][9]    | 0/1                                                      |
| captainOrderLoc[2] | captain's slot in the batting order, 0-8                 |
| logo[2]            | team logo id, 0-0x2F                                     |
| stadiumCursor      | stadium-select *cursor* index, mapped through the game's own `cursorToStadIDMapping`: 0=Mario 1=Bowser 2=Wario 3=Yoshi 4=Peach 5=DK. Toy Field is not on this table; a Toy Field boot needs the `selectRandomStadium` toy-field patch instead. Written to `g_d_GameSettings.StadiumID` (`0x800E8705`). |
| firstBatter        | 0 = P1 bats first, 1 = P2. This is who bats when the match *starts*; the game makes that team the in-game away side. For a bottom-of-inning boot this must be the true home player (they bat when play resumes), paired with swapped `logo[]` slots so the scoreboard rows stay true. |
| starSkills, innings, mercy | mercy writes `runsNeededForMercy = 10` (MSSB mercy = 10 runs); 0/off handling is unverified |
| isCpuMatch         | 1 = P1 vs CPU. Unverified in the force-swap flow; the tested path is two humans. |
| p2Port             | physical port of the second human, 2-4 (1-based)         |

### BootStateConfig

Where in a game to land. Applied every pre-pitch frame because game init
overwrites these partway through the load; `hasGameStarted_` is 0 only during
that load, so the gate is self-limiting.

| field              | meaning                                                  |
| ------------------ | -------------------------------------------------------- |
| apply_state        | 0 = fresh match, 1 = jump to the state below             |
| inning             | 1-based; keep <= the spec's `innings` (a value past regulation boots into extras and the loader never stages that BGM) |
| bottom_of_inning   | 0 top, 1 bottom; also sets `homeTeamBattingInd_fieldingTeam` / `awayTeamBattingInd_battingTeam` |
| score_away/home    | totals, also written to inning 1's box                   |
| balls, strikes, outs, stars_p1/p2, star_chance | as named                     |
| apply_order        | 0 = leave the game's default order alone                 |
| orderChar_A_H[2][9], orderPos_A_H[2][9] | batting order and fielding position per slot, AWAY/HOME order, current-batter-first |
| currentBatter_A_H[2] | current batter's slot, 1-indexed                       |
| orderFieldHand/BatHand/Superstar_P1P2[2][9] | per natural batting slot, P1/P2-indexed |
| runner_present/rosterLoc/charID[3] | 1B/2B/3B                                 |
| positionSwap[2][9] | `positionSwapMapping`: the fielding position each roster slot plays, in draft order. Staged verbatim; identity does not work because a position-order roster feeds the fielder-table build wrong. |

`characterSelectScreen()` only builds a default order from the position-ordered
roster, so the real batting order, the fielder at each position and the current
batter are all wrong for a mid-inning boot. The config overwrites
`g_GameLogic.battingOrderAndPositionMapping[team][slot][0..1]` directly: slot 0
is the pitcher copy (position 0), slots 1-9 are the batting order, `[0]` is the
batter's index into the natural-order `inMemRoster` (which is just `i`, since
the char hook builds `inMemRoster` in that same order) and `[1]` the position.
Handedness/superstar are then written into `inMemRoster[team][i].stats`
(`FieldingArm`, `BattingStance`, `UnusedBytes[0]`) after that reorder; doing it
in position order during staging would land them on the wrong players.

`g_Scores` (`GameScoresControlsStruct`, `0x808928A0`) is bound by address
because this code touches a game-REL object while bound to the menu REL's
symbols; `Include/Symbols` refuses to bind both (they share an arena slot).

### Runners on base

| item                     | address                              |
| ------------------------ | ------------------------------------ |
| runner roster loc (i)    | `0x8088F04C + i*0x154` (u16)         |
| runner charID (i)        | `0x8088F04E + i*0x154` (u16)         |
| clear instruction (i)    | `0x806C9420 + i*0x30`                |
| original clear (1B)      | `0xB0650234`                         |
| original clear (2B/3B)   | `0xB06500E0`                         |

The clear instruction stores the runner's roster id to 0 during game init.
It is NOP'd (`0x60000000`) while the load runs so a staged runner survives,
and restored once `EventTriggers_GameHasStarted` is set.

### Outs are applied late

Booting with outs already on the board silences the first at-bat's BGM
(confirmed in both the menu-walk and force-swap flows, 2026-07-20). So the
match starts clean and the outs go on after `TBM_OUTS_DELAY_FRAMES` (180),
written for `TBM_OUTS_BURST_FRAMES` (10) frames, and never once
`g_Stats.atBatPitchThrown` is set. Counter: `0x802EC01A` (u8), shared with
Boot Directly To Game's since the two never run together.

## Boot To Main Menu

### Part 1: skipping intro and title

The boot sequence is a per-frame task (`0x8063F904+`) that walks the boot
screens through a jump table at `0x806D7118`: substate 7 -> screen 1 (the title
screen, which performs the memory-card check) and substate 9 -> screen 5 (the
main menu). Forcing the substate-9 dispatch to `li r3, 5` jumps straight to the
main menu, which is the original gecko:

    280E877C 00000000   if half[rel] == 0
    0463F964 38600005      [0x8063F964] = li r3, 5
    E2000001            endif

### Part 2: loading the slot-A save the title screen would have

The load is the game's memory-card check, queued by `0x8003F23C`: a 35-state
async task that mounts slot A, finds or creates the Mario Baseball file, reads
it, and unmounts, staging through ARAM and scattering the data into the live
save structures. It is the only practical loader (the save is unpacked
field-by-field into dozens of globals). Set `0x80366177 = 1` before firing it.
The catch: it is a user-facing wizard that posts a dialog at nearly every state.

Desired behaviour matrix:

| situation                 | result                          |
| ------------------------- | ------------------------------- |
| card with MSSB data       | load it                         |
| no memory card            | silently ignore and continue    |
| card but no MSSB data     | silently create MSSB data       |
| not enough space          | silently ignore and continue    |

Two methods, selected by `CARD_LOAD_METHOD`:

- `CARD_LOAD_GAME_WIZARD` (0): just fire `0x8003F23C`. Proven loader, but
  shows "loading from Slot A" and yes/no prompts. Kept as reference/fallback.
- `CARD_LOAD_SILENT` (1, the shipped one): probe slot A first with the SDK
  `CARDProbeEx(chan, *memSize, *sectorSize)` at `0x80086144` (0 = ready, -1 =
  busy/still detecting, -3 = no card; needs no mount, the game uses it the same
  way at `0x800ABBA0`) and only invoke the wizard when a card is present, then
  hide the wizard's dialog and auto-confirm its prompts for ~600 frames.

The wizard's dialog lives in the card-check struct at `0x803C50E8`:

| offset | address      | meaning                                                  |
| ------ | ------------ | -------------------------------------------------------- |
| +0x3e  | `0x803C5126` | "dialog active" flag the on-screen box reads every frame |
| +0x44  | `0x803C512C` | response field A                                         |
| +0x45  | `0x803C512D` | response field B; the wizard branches on 3-4 = proceed/confirm |

Holding +0x3e at 0 hides the box; writing 1 / 3 into the response fields
auto-answers the "create?" prompt.

State kept in claimed RAM: `0x802EC01B` (u8) is a persistent "already loaded"
latch, set once a load succeeds and never cleared, so the load runs once per
session rather than every menu reload (DOL .bss is zeroed at cold boot but not
when only the menu REL reloads). It must not be `0x803C50E8`: the card task
reads that u8 as an abort flag. `0x802EC01E` (u8) is the per-menu-load phase
(0 probe, 1 suppression window, 2 idle), re-armed every `rel == 0` frame so a
boot with no card re-checks on the next menu load. `0x802EC01C` (halfword) is
the frame counter.

The code has no `.state` because it must see rel 0 (force the boot walk) and
rel 4 (drive the card load).
