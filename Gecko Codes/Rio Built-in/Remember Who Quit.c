/*###########################################################
# Remember Who Quit
###########################################################*/
// Author: LittleCoaks

// Both quit paths overwrite `sth r4, 254(r3)` with r0 (= 8) live until the
// `stb r0` that follows, so the C wrapper cannot be used: ASM bodies.
// Claimed bytes (ClaimedFreeMemory.h): 0x802EBF93 who quit, +1 fielder port, +2 batter port.

// Quit while batting
ASM(RememberWhoQuit_Batter,
    "sth  4, 254(3)       \n"     /* overwritten instruction */
    "lis  20, 0x802E      \n"
    "ori  20, 20, 0xBF93  \n"
    "lbz  21, 2(20)       \n"
    "stb  21, 0(20)       \n",
    .address = 0x806EDF88, .state = MSSB_GAME,
    .notes = "Records which player quit a match so Rio can report it.");

// Quit while fielding
ASM(RememberWhoQuit_Fielder,
    "sth  4, 254(3)       \n"     /* overwritten instruction */
    "lis  20, 0x802E      \n"
    "ori  20, 20, 0xBF93  \n"
    "lbz  21, 1(20)       \n"
    "stb  21, 0(20)       \n",
    .address = 0x806ED700, .state = MSSB_GAME);
