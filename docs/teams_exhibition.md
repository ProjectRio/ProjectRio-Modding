# Teams Exhibition: rotation and the HUD port icons

Used by: `Gecko Codes/Global/Teams Exhibition.c`.

## Game type

The drafting port decides the mode: `g_d_GameSettings.PlayerPorts[1]` = 1 ->
1v2 (P1 vs P2/P3), 2 -> 2v2 (P1/P2 vs P3/P4), 3 -> 1v3 (P1 vs P2/P3/P4).
Anything else (vs CPU) leaves the code inactive. Team 0 starts at port 0; team
1 fills the ports immediately after it, so its base is team 0's size, not
`PlayerPorts[1]`.

## Rotation

Plate appearances by the batting team (sum of
`Static_Stats_Tables.batterStats[bat_team][0..8].plateAppearances`; unlike
AtBats this counts walks) drive both the batter round-robin and the 3-player
pitcher/fielder rotation, since they double as the count of batters the
fielding team has faced.

- Batting: round-robin through the batting team's players each plate
  appearance (size 1 -> the one player; 2 -> A/B toggle; 3 -> P2/P3/P4).
- Fielding, one defender: pitches and fields.
- Fielding, two defenders: split pitcher/fielder and trade roles each inning
  (`is_odd_inning == is_at_bat` picks fielder A).
- Fielding, three defenders: rotate the pitcher/fielder pair each batter
  faced; the third rests. Each player cycles pitch -> field -> rest.

`SCENE_ID_REPLAY_AT_BAT` counts as at-bat for the pitcher/fielder role: the
input hook never runs during replays, but the HUD icon hook can.

## Handing over control (hook 0x806AC530, end of UpdateControllerInputs)

The game reads team 0 from port 0 and team 1 from the drafting port (the
untouched `teamPorts` values; never write `teamPorts_P1P2`). The hook
overwrites the read port's `g_Controls` struct (0x10 bytes, copied as four
words) with the active teammate's. A whole-struct copy keeps
`newInputOnLatestFrame` edges coherent, since they were computed against the
source port's own history. The hook freezes during replays: playback feeds
the recorded inputs back through the unmodified mapping, and copying live
inputs over them would corrupt the replay.

`0x802EBF94/5` (`autogolf_ports`) receive the active fielder/batter port + 1.
This is temporary: current Rio client versions read it for autogolf mode;
remove once the client no longer does.

## HUD port icons (hook 0x806D8EC0, draw_ongoingStarGuageHud)

`draw_initStarGuageHud` picks each team's port icon from `g_GameLogic.teams`
when the HUD is created and never updates it, so the icon goes stale as the
rotation advances. The hook runs every frame the HUD is drawn and re-writes
the icon frame the same way init does:

| step      | value                                                  |
| --------- | ------------------------------------------------------ |
| manager   | `*(0x803CC1B8)`; slot index = u16 at manager+0x14      |
| entry     | `0x80371C30 + index*8` (graphics object pair table)    |
| objects   | `*(entry+0x78)` fielding icon, `*(entry+0x80)` batting icon |
| icon u32  | object+0x5C, value `port << 16` (frames 0-3 = human 1P-4P; +4 = the CPU variants) |

Injected at an idle `li r5, -1` inside the ongoing draw so the objects are
guaranteed to exist. Both hooks are `.state = MSSB_GAME`: every address is
game-REL code.
