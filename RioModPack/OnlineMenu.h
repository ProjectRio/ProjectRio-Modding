/*###########################################################
# OnlineMenu.h -- what the rest of the pack needs to know about the Online button
###########################################################*/
// Author: LittleCoaks
//
// The Online placeholder screen lives on a screenCode of its own, and two other
// parts of the pack have to recognise it: the Options Menu's background
// watchdog (which un-blanks the UI whenever "our" screen is not the current
// one) and the Dictionary music swap's list of main-menu screens. Both include
// this header rather than the mod itself.

#ifndef ONLINEMENU_H
#define ONLINEMENU_H

/* screenFuncTable[2] and [3] are the game's "removed step" assert stub
 * (0x806402B0: an OSPanic that no stock code ever reaches). The mod replaces
 * that handler, so screenCode 2 becomes the Online screen. It is reached
 * only through the mod's own reroute of the Options transition, never by the
 * game, and it never has to survive a menu reload: leaving it puts the main
 * menu back exactly the way the Options screen does. */
#define ONLINE_SCREEN_CODE 2

#endif /* ONLINEMENU_H */
