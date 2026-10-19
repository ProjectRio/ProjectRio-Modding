/*###########################################################
# Clear Hit Result
###########################################################*/
// Author: PeacockSlayer

// ASM: r0 and cr0 are both live across this site (lastPlayStats stores r0 and
// branches on cr0 right after), so the C wrapper cannot be used here.
ASM(ClearHitResult,
    "stb  8, 55(9)        \n"     /* overwritten instruction; r8 == 0 here */
    "lis  21, 0x8089      \n"
    "ori  21, 21, 0x3BAA  \n"     /* the stored hit result Rio's stat tracker reads */
    "stb  8, 0(21)        \n"
    "li   21, 0           \n",
    .address = 0x806BBF88, .state = MSSB_GAME,
    .notes = "Clears the stored hit result at the start of each play so Rio's stat tracking reads it fresh.");
