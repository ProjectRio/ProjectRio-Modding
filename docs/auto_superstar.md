# Auto Superstar

Used by: `Gecko Codes/Menu/Auto Superstar.c` (hook at `0x8005A4F4`, the
team-management superstar walk, plus a second hook at `0x800625A4` that
restarts it on every screen entry).

The code marks every character on both teams for superstardom, then walks
each team's roster cursor and triggers the game's own superstarring step for
each one. Self-contained: no other tool or code needs to drive it.

Requirements:

1. Which characters to superstar is read from the first unused byte of each
   character's entry in the roster struct. The first indicator byte is at
   `0x80353BE5` (first character on the P1 team); each character is 0xA0
   apart, P2's team starts 9 characters in. The code writes all 18 bytes to 1
   itself, every call, so they self-heal if anything else touches them; the
   old standalone way to do this by hand was the raw write

       08353be5 00000001
       001100a0 00000000

2. Two free bytes to track progress, one per team: `0x802EBF99` and
   `0x802EBF9A` (claimed RAM; `0x802F0000 - 0x4067` in the code). Index 0 =
   not started (incremented once to give superstar codes a chance to load),
   1..9 = the roster slot being starred, 0xA = done, park the cursor at 0;
   0xB = nothing left to do. This is claimed scratch memory, not a real game
   field, so nothing resets it on its own -- it needs an explicit "team
   management was just entered" trigger, or the walk only ever completes
   once, on the first visit after boot.

   That trigger is `updateCharacterSelectProcessCode(team, code)`
   (`0x800625A4`, decomp: `File_0x800625a4.c`/`.h`): a "post a menu process
   code" call used throughout the unreversed captain-select/team-management
   flow. Verified live: `code == 0x38` is posted once per team (twice total)
   every time team management is (re-)entered, including a revisit after
   backing out to character select -- unlike every other candidate tried
   (see "Dead ends" below). Hooking its entry to reset both progress bytes
   on `code == 0x38` is what makes the walk restart on every (re-)entry.

   Dead ends tried first, for anyone who hits the same wall again:
    - Hooking `createTeamManagementScreen_preGame` (`0x80048764`) directly.
      Confirmed live via a diagnostic counter that this function's entry is
      invoked continuously, every frame, for the entire time the screen is
      active (not once on entry) -- its own one-shot counter only gates its
      *body*, which is why it needs one at all. A hook at its entry resets
      the progress byte every frame, so the walk index can never advance
      past 0: this broke starring entirely, including the first visit, not
      just the restart.
    - The first live capture of `updateCharacterSelectProcessCode` codes
      ended in a run of distinct high values (0x3B-0x3F) right before
      team management appeared, and `0x3F` looked like the trigger -- but a
      second capture on a revisit showed only `0x38, 0x38` with none of the
      higher codes at all. Those higher codes are one-time load/setup steps
      that don't repeat, the same trap as the function-entry approach above,
      just relocated. `0x38` was the only thing posted on both a first visit
      and a revisit, confirmed by testing the actual reset on it.

Game state it touches: the team-management cursor at `0x80336726 + team`,
and the "superstarring in progress" byte at `0x8033677E + team`, which the game
reads to apply the superstar to the character under the cursor. Both are
indexed by team (`byte + team`), so P2's in-progress byte is `0x8033677F`;
the decomp header currently types `inProgress_superStarAPlayer` as a single
byte.

The old asm used r24 as scratch. `battingOrderProcessInputs` (the hooked
function, `0x8005A350`) only saves r26-r31, so that clobbered the caller's
r24; the C body keeps every register.

Note: the Auto-Superstar walk does not fire in a force-swap boot (Instant
Randoms / Boot To Match); those drive `transferStatsToInMemRoster` directly
(see `boot_to_match.md`).
