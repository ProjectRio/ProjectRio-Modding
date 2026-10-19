/*###########################################################
# Gecko Codes
###########################################################*/
// Author: LittleCoaks
// See docs/mod_options.md ("The Gecko Codes toggle").
#include "Include/game/UnknownHomes_Game.h"
#include "RioModPack/ModOptions.h"

// Dolphin/Rio's Gecko constants (Source/Core/Core/GeckoCode.h).
#define GECKO_INSTALLER_BASE 0x80001800
#define GECKO_ENTRY          0x800018A8   // INSTALLER_BASE_ADDRESS + 0xA8

#define GECKO_ENTRY_STOCK    0x9421FF54   // stwu r1, -0xAC(r1)
#define GECKO_ENTRY_OFF      0x4E800020   // blr

// The installed handler's first word reads MAGIC..MAGIC+5; anything else means no handler.
#define GECKO_MAGIC       0xD01F1BAD
#define GECKO_MAGIC_SPAN  5

CGECKO(GeckoCodes, .state = MSSB_ALWAYS,
       .notes = "Turns your extra gecko codes on or off. This build's own mods "
                "are unaffected, but Rio's built-in codes are turned off too.");
void GeckoCodes()
{
    u32 magic;

    ModOptions_ApplyDefaults();

    magic = VAR_ADDRESS(u32, GECKO_INSTALLER_BASE) - GECKO_MAGIC;
    if (magic > GECKO_MAGIC_SPAN)
        return;                                  // no handler installed

    if (ModOptionOn(MODOPT_GECKO))
        PatchInstruction_Conditional(GECKO_ENTRY, GECKO_ENTRY_OFF, GECKO_ENTRY_STOCK);
    else
        PatchInstruction_Conditional(GECKO_ENTRY, GECKO_ENTRY_STOCK, GECKO_ENTRY_OFF);
}
