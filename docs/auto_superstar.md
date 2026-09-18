# Auto Superstar

Used by: `Gecko Codes/Menu/Auto Superstar.c` (hook at `0x8005A4F4`, the
team-management superstar walk, plus a second hook at `0x80048764` that
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
   field, so nothing resets it on its own between visits -- it only defaults
   to 0 once, at boot.

   `createTeamManagementScreen_preGame` (`0x80048764`) is the game's own
   screen-creation callback for team management (verified live: its first
   instruction is `stwu r1, -0x10(r1)`, `0x9421fff0`); it also resets a few
   sibling fields in the same struct as the cursor/in-progress bytes above,
   which is how it was identified. Hooking its entry to reset both progress
   bytes back to 0 is what makes the walk restart every time the screen is
   (re-)entered, e.g. backing out to character select and returning -- not
   just the first time after boot. Only the pre-game create path is hooked;
   the mid-match `createTeamManagementScreen_inGame` (`0x800486E0`) is left
   alone, so manual mid-game roster edits aren't clobbered by a restart.

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
