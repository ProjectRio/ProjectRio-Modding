/*###########################################################
# Remember Who Quit
###########################################################*/
// Author: LittleCoaks

// Both quit paths overwrite `sth r4, 254(r3)` with r0 (= 8) live until the
// `stb r0` that follows, so the C wrapper cannot be used: ASM bodies.
// Stores g_d_GameSettings.PlayerPorts[teamBatting / teamFielding] (the quitting team's 0-based port) to 0x802EBF93.

// Quit while batting
ASM(RememberWhoQuit_Batter,
    "sth  4, 254(3)       \n"     /* overwritten instruction */
    "lis  20, 0x8089      \n"
    "lwz  21, 0x2990(20)  \n"     /* g_GameLogic.teamBatting */
    "lis  20, 0x800E      \n"
    "ori  20, 20, 0x874C  \n"     /* g_d_GameSettings.PlayerPorts */
    "lbzx 21, 20, 21      \n"
    "lis  20, 0x802E      \n"
    "ori  20, 20, 0xBF93  \n"
    "stb  21, 0(20)       \n",
    .address = 0x806EDF88, .state = MSSB_GAME,
    .notes = "Records which player quit a match so Rio can report it.");

// Quit while fielding
ASM(RememberWhoQuit_Fielder,
    "sth  4, 254(3)       \n"     /* overwritten instruction */
    "lis  20, 0x8089      \n"
    "lwz  21, 0x2994(20)  \n"     /* g_GameLogic.teamFielding */
    "lis  20, 0x800E      \n"
    "ori  20, 20, 0x874C  \n"     /* g_d_GameSettings.PlayerPorts */
    "lbzx 21, 20, 21      \n"
    "lis  20, 0x802E      \n"
    "ori  20, 20, 0xBF93  \n"
    "stb  21, 0(20)       \n",
    .address = 0x806ED700, .state = MSSB_GAME);
