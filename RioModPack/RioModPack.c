/*###########################################################
# RioModPack
###########################################################*/
// Author: LittleCoaks
// One code bundling the Options menu and every mod it can toggle.
// Gate wiring per mod: docs/mod_options.md.
#include "Include/game/UnknownHomes_Game.h"
#include "RioModPack/ModOptions.h"

// ---- the options UI itself, never gated -- it is how you reach the toggles --
#include "RioModPack/Options Menu.c"
#include "RioModPack/Online Menu.c"
#include "Gecko Codes/Global/Boot To Main Menu.c"

// Self-gates: patches game code, so it must keep running while OFF to restore it.
#undef  CGECKO_OPTION_ADDR
#define CGECKO_OPTION_ADDR MODOPT_ADDR(MODOPT_DUPLICATES)
#undef  CGECKO_ACTIVE
#define CGECKO_ACTIVE ModOptionOn(MODOPT_DUPLICATES)
#include "Gecko Codes/Menu/Duplicate Characters.c"
#undef  CGECKO_ACTIVE
#define CGECKO_ACTIVE 1
#undef  CGECKO_OPTION_ADDR
#define CGECKO_OPTION_ADDR CGECKO_GATE_ADDR

// Self-gates for the same reason (it patches the emulator's code handler).
#undef  CGECKO_OPTION_ADDR
#define CGECKO_OPTION_ADDR MODOPT_ADDR(MODOPT_GECKO)
#include "RioModPack/Gecko Codes.c"
#undef  CGECKO_OPTION_ADDR
#define CGECKO_OPTION_ADDR CGECKO_GATE_ADDR

// ---- toggleable mods -------------------------------------------------------
#undef  CGECKO_GATE_ADDR
#define CGECKO_GATE_ADDR MODOPT_ADDR(MODOPT_WIDESCREEN)
#include "Gecko Codes/Global/Widescreen.c"

#undef  CGECKO_GATE_ADDR
#define CGECKO_GATE_ADDR MODOPT_ADDR(MODOPT_NIGHT_MARIO)
#include "Gecko Codes/Menu/Nighttime Mario Stadium.c"

// The ASM swing hook is gate-wrapped; the instruction patches self-gate.
#undef  CGECKO_GATE_ADDR
#define CGECKO_GATE_ADDR MODOPT_ADDR(MODOPT_SWING_SKIP)
#undef  CGECKO_ACTIVE
#define CGECKO_ACTIVE ModOptionOn(MODOPT_SWING_SKIP)
#include "Gecko Codes/Match/Skip First Swing Frame.c"
#undef  CGECKO_ACTIVE
#define CGECKO_ACTIVE 1

// Configured, not switched: never gated, only keyed for its notes.
#undef  CGECKO_GATE_ADDR
#define CGECKO_GATE_ADDR 0
#undef  CGECKO_OPTION_ADDR
#define CGECKO_OPTION_ADDR MODOPT_ADDR(MODOPT_MUSIC)
#include "RioModPack/Custom Music.c"

// Active only while the menu slot selects Dictionary; must keep running to restore fx 484.
#undef  CGECKO_ACTIVE
#define CGECKO_ACTIVE (MusicSlot(MUSIC_SLOT_MENU) == MUSIC_DICTIONARY)
#define DMM_SCREENS(sc) ((sc) == 5 || (sc) == 6 || (sc) == ONLINE_SCREEN_CODE)
#include "Gecko Codes/Menu/Dictionary Replaces Menu Music.c"
#undef  CGECKO_ACTIVE
#define CGECKO_ACTIVE 1

#undef  CGECKO_OPTION_ADDR
#define CGECKO_OPTION_ADDR CGECKO_GATE_ADDR

#undef  CGECKO_GATE_ADDR
#define CGECKO_GATE_ADDR 0
