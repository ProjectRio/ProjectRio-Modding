# Auto Superstar

Used by: `Gecko Codes/Menu/Auto Superstar.c` (hook at `0x8005A4F4`, the
team-management superstar walk).

The code walks each team's roster cursor and, for every character flagged for
it, triggers the game's own superstarring step. It is meant to be driven by
another tool or code, not enabled on its own.

Requirements:

1. Which characters to superstar is read from the first unused byte of each
   character's entry in the roster struct. The first indicator byte is at
   `0x80353BE5` (first character on the P1 team); each character is 0xA0
   apart, P2's team starts 9 characters in. For the common case of starring
   everyone:

       08353be5 00000001
       001100a0 00000000

2. Two free bytes to track progress, one per team: `0x802EBF99` and
   `0x802EBF9A` (claimed RAM; `0x802F0000 - 0x4067` in the code). Index 0 =
   not started (incremented once to give superstar codes a chance to load),
   1..9 = the roster slot being starred, 0xA = done, park the cursor at 0;
   0xB = nothing left to do.

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
