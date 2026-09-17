# Match codes: engine notes

Engine facts behind the smaller `Gecko Codes/Match/` mods, kept out of the
sources. All addresses are game.rel as Rio loads it (`.text` at 0x8063F094;
the decomp's `mapped:` comments use the same base).

## Hazardless Stadiums (`0x80699508`, fn_3_5A28C+0x1D0)

The hook sits right after the stadium-load code reads `g_d_GameSettings.StadiumID`
(`lbz r5, 9(r4)`), so it runs once per match load, after the REL has been
reloaded with stock code. Each stadium's hazard controller is switched into its
minigame mode (`li r0, 7` in the load functions), which loads no hazards:

| Stadium | Site | Patch |
|---|---|---|
| Wario | palaceNadoLogic `0x8070FC30` | `nop` (tornado spawn) |
| Wario | palaceChainChompControl `0x8071339C` | `li r0, 0` (chomps freeze) |
| Wario | `TornadoPlacementConfig` +0x00 / +0x34 | 100.0f: the tornado anchors are parked off the field |
| Bowser | flameControl `0x807056C8` | `li r3, 0` |
| Bowser | thwomp_slamControl `0x80706D00` | `li r0, 0` |
| Yoshi | loadYoshiPark `0x80724428` | `li r0, 7` |
| Peach | loadPeachGarden `0x80739A28` | `li r0, 7` |
| DK | loadDKJungle `0x80736D08` | `li r0, 7` |
| DK | handleBarrelFiring `0x807343A4` / `+0xC` | `nop` (barrels ignore minigame mode) |

Wario's minigame mode still spawns a single chain chomp at home plate, which is
why it is handled instruction by instruction instead.

## Highlight Ball Shadow (`0x806A844C`, ballAnimationSubFun2+0xC0)

`ballAnimationSubFun2` draws the drop-spot marker. Its first check is the
batting port's drop-spot setting (`cmplwi r4, 0; beq <return>`). The hook
replaces the `beq` and never takes it, so the marker also draws with the
setting off; in that case it patches the two position loads that feed the
marker (`lfs f0, 0x348(r29)` at `0x806A85B8` -> `lfs f0, 0(r29)`, and
`lfs f0, 0x34C(r5)` at `0x806A85D4` -> `lfs f0, 8(r5)`, r5 = `g_Ball`) so the
marker follows the ball itself: a high-contrast shadow. With the setting on the
original code path runs untouched.

## Hold Z To Show Easy Batting (`0x806A82B0`, ballAnimationSubFun3+0x80)

The replaced instruction is `cmplwi r0, 1` on the practice easy-batting flag;
`beq` continues into the guide path, otherwise the function returns. The hook
computes "Z held by the batting port" (`g_Controls[teams[teamBatting]].buttonInput`)
and re-creates the compare on r4 (`cmplwi r4, 1`, r4 = 0 when Z is held, 1 when
not), which keeps the shipped behaviour: equal while Z is up. r4 and r5 are
both reloaded before their next use on either path, and r0 is not read again.

## Remove Seagulls (per frame)

`loadMarioStadium+0x30` (`0x80708E90`) is `stw r5, 0x1C(r6)`, the store of
the seagull asset pointer into the stadium object at `0x808961C4`. Dropping it
leaves the pointer empty and no seagulls are created. The REL is reloaded each
match, so the patch is re-applied per frame while game.rel is resident.

## Load Savestate / restart at-bat (`0x806AA1F0`, animateDefence+0x18)

Hooks the `lbz r4, 0x11E(r27)` that reads `g_GameLogic.gameStatus` at the top
of `animateDefence` (r27 = `g_GameLogic`). While the status is
`GAME_STATUS_LIVE_BALL`, L + Z on any port sets it to
`GAME_STATUS_TRANSITION_PREPARE_NEXT_PLAY` (7), which is the transition the
replay system uses to restart the play. The re-run `.instruction` then loads
the new status for the function's own tests.

## Z + C Stick To Increase or Decrease Your Score (`0x8069A15C`, gameSimulationFunction+0x4)

Runs once per simulated frame off the prologue's `mflr r0`; the wrapper's
`mtlr` restores LR before the `.instruction` re-runs, so r0 is correct. The
batter (`teamBatting`) edits `g_Scores.scores[halfInning]`, the pitcher
(`teamFielding`) the other side. Ports >= 4 are CPU/none. Z is read from
`AtBat_ButtonInput1` (new presses this frame, one u16 per port at stride 0x20,
`0x803C77BA`), the C-stick from the raw controller records below.

### Raw controller records at 0x802E9F40

`g_InputBuffer + 0x20` holds the four raw SI poll results, 8 bytes per port:
buttons hi, buttons lo (bit 7 always set in raw data), stick X/Y, C-stick X/Y,
L/R triggers. It is not a `PADStatus` array (that would be 12 bytes per port);
the synced header's `PADStatus pads[4]` at that offset is wrong and should be
fixed in the decomp. Both codes above depend on the 8-byte stride and, for the
L + Z test, on bit 7 being set (`& 0xD0 == 0xD0`).

## Batter Lag Reduction & Positional Correction (`BattingPrediction.h`)

The implementation lives in `Gecko Codes/Match/BattingPrediction.h` and is
shared with `Input Prediction.c`, which adds a pitching half
(`docs/import_match_rules.md`). Each `.c` only supplies its player description
(`BATTING_PREDICTION_NOTES`) before including the header. The two codes hook
the same sites, so only one of them may be enabled at a time.

Converted from a hand-written ini. The ini wrapped the three lag-reduction
pieces (not the positional correction) in `2289091C 00000000`, a 32-bit
**if-not-equal**: they apply only once `g_Batter.batPosition.y`
(`0x8089091C`) is non-zero, i.e. a batter has actually been placed. A gecko
`if` around a `C2` only decides when the branch is installed (it stays
installed afterwards), so for the hooks the condition is just a crude "match is
live" check that `.state = MSSB_GAME` already covers. It only matters for the
per-frame writes, which keep the condition as `BatterIsPlaced()`. Four pieces:

- **Frame patches (per frame).** `hittableFrameInd[0][1] = 1` lets an
  uncharged swing make contact on frame 1 (stock: frames 2..10). The
  `cmpwi r0, 1` tests at `0x806A3688` (batterAnimations) and `0x80654610`
  (assignFrameCountersToAPointer) move to `== 2`, the same two sites Skip First
  Swing Frame patches.
- **calculateIfHitBall entry (`0x80651D48`).** If `framesSinceStartOfSwing`
  is 1 it becomes 2, together with `Stored_Frame_SwingContact_SinceMiss` and the
  byte at `0x80893303` (decomp `lbl_3_common_bss_32220+3`, meaning unknown),
  so a first-frame swing is judged as frame 2. The original tested and wrote
  only the low byte.
- **ifSwing (`0x80652360`).** `frameSwung = g_Ball.pitchHangtimeCounter - 1`
  (unless the counter is above 0xFFF). ASM: the value returns in r0.
- **batterInBoxMovement (`0x80652D00` X, `0x80652D9C` Z).** Replaces the
  `fadds f0, f0, f2` that adds this frame's box movement (f2) to
  `batPosition2` (f0). With the previous change `p` kept in a claimed word,
  the applied change becomes `2*f2 - p`: the position the input would have
  reached had it arrived a frame earlier. The previous position is also kept;
  if the game moved the batter by other means (mismatch), `p` resets to 0
  first. Claimed words: `0x802EBF9C` = 0.0f, `0x802EBFA0/A4` previous X/Z
  change, `0x802EBFAC/B0` previous X/Z position. ASM because the result must
  leave in f0 and the C wrapper restores every FPR a C body touches. The
  conversion uses r8/f12/f13, none of which are live in the function; the
  original clobbered r19/f28/f29 and then zeroed them, wrote its 2.0f constant
  into `g_Batter.batterPos.x` as scratch, and on the Z axis reset `p` to the X
  change word instead of 0.0f. All three were fixed.
