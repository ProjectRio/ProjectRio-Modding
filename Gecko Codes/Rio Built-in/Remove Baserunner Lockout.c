/*###########################################################
# Remove Baserunner Lockout
###########################################################*/
// Author: nuche17, LittleCoaks

// ASM: the lockout length is the r0 that running_MainFunction compares next
// (`cmpw r3, r0`), so the result has to come back in r0 itself.
ASM(RemoveBaserunnerLockout,
    "lha  0, 6(29)        \n"     /* overwritten instruction: frames until runners unlock */
    "lis  14, 0x8089      \n"
    "ori  14, 14, 0x2701  \n"     /* g_Ball.ballState */
    "lbz  14, 0(14)       \n"
    "cmpwi 14, 0          \n"
    "bne  1f              \n"
    "li   0, 1            \n"     /* ball not yet in play: unlock on the next frame */
    "1:                   \n"
    "nop                  \n",
    .address = 0x806C9D78, .state = MSSB_GAME,
    .notes = "Runners can be controlled right after contact instead of after the usual short lockout.");
