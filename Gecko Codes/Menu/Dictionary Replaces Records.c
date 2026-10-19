/*###########################################################
# Dictionary Replaces Records
###########################################################*/
// Author: LittleCoaks
// The stock-scene version: reroute + music fix only. For the custom scene
// with added elements, use "Dictionary Scene".

#include "Include/Rio/DictionaryReroute.h"

CGECKO(DictionaryReplacesRecords, .state = MSSB_MENU,
       .notes = "Makes the Records button on the main menu open the game's\n"
                "unused Dictionary scene instead of Records.\n"
                "Turn on this or \"Dictionary Scene\", not both.");
void DictionaryReplacesRecords()
{
    DictionaryReroute_Tick();
}
