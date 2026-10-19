# Manual Fielder Select

`Gecko Codes/Ranked/Manual Fielder Select.c` is the C port of the v5.0 asm
(v4.1, v5.0 and a half-finished C port used to ship side by side, all hooking
the same instruction). This is the engine knowledge the port rests on.

## The hook site

`0x80678F8C` in game.rel is `lbz r0, 0x1BD1(r6)` with `r6 = &g_Ball`, i.e. it
reads `g_Ball.deadBallReason`. The game's fielder-assignment routine then does
`cmplwi r0, 0; bne <epilogue>`: a non-zero dead-ball reason means "nothing to
assign this frame" and the function returns at once.

r0 is not live across the site (the previous r0 was consumed two instructions
earlier), so a C body is legal. What the asm did with r0 -- `li r0, 1` on its
"fake" exit so the game skips its own pick -- a C body cannot, because
cgecko's wrapper owns r0. Instead the port rewrites the saved **r6** so the
re-run `.instruction` reads our own state byte: `r6 = 0x802EBF97 - 0x1BD1`.
That byte (`MFS_STATE`) is non-zero on every path that wants the skip (1 =
selecting, 2 = undo held), so the game branches straight to the epilogue,
where r6 is dead (`lwz r0/r31/r30; mtlr; addi r1; blr`). On the "real" exit r6
is left alone and the game reads `deadBallReason` as it always did.

## State

Two claimed bytes (`ClaimedFreeMemory.h`):

- `0x802EBF96` -- the button state seen last frame (0 none, 1 R, 2 Z), used to
  tell a fresh press from a hold. Holding keeps the current selection and
  keeps the game from re-picking; it does not re-run the closest search.
- `0x802EBF97` -- the active selection state, same encoding.

The v5 asm also wrote 0xFF to `0x802EBF99` on reset. Nothing ever read it, and
that byte now belongs to the superstar character-code mods, so the port drops it.

## Flow (per frame the hook runs)

1. `g_Ball.framesSinceHit == 4` -> clear both bytes (no carry-over from the
   previous at-bat), let the game run.
2. Fewer than 3 outs and the ball is neither `BALL_STATE_HIT` nor
   `BALL_STATE_LOOSE` -> clear both bytes, let the game run. After the third
   out a select is still allowed (moonwalking).
3. Star swing with the ball still in the air -> deselect, let the game run.
4. Read `g_FieldingLogic.fielderInputs`. Z sets state 2, R sets state 1 (R
   wins when both are down). Held = same raw press as last frame.
5. State 0 -> game runs. Held -> skip the game's pick, keep the selection.
   State 2 (undo) -> clear state, game runs.
6. Target is the ball (`g_Ball.AtBat_Contact_BallPos`) once the contact result
   is `LANDED`, else the landing spot
   (`g_Ball.physicsSubstruct.ballLandingSpotOrHeldSpot`). Distance is
   |dx| + |dz| (the asm doubled both terms; ordering is identical). The
   fielder in `g_FieldingLogic.selectedFielder` is never a candidate; ties go
   to the later slot.
7. `fielderLockout[fielder] > framesSinceHit` -> deselect, game runs.
8. Every fielder at `AUTO_MOVEMENT_GOING_TO_BALL` (0xF) is dropped to
   `AUTO_MOVEMENT_TRACK_HIT_BALL_PHASE2_AI_TEAM` (2, "work out your own
   state"), the chosen one is set to 0xF, `selectedFielder` and
   `selectedFielder_stored` are set, and the game's pick is skipped.

The asm wrote only the low byte of the two s16 fielder fields; the port writes
the whole halfword.
