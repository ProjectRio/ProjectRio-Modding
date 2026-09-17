/*###########################################################
# Duplicates & Variants Have Chemistry
###########################################################*/
// Author: PeacockSlayer, LittleCoaks
#include "Include/Rio/StatEdits.h"

#define FULL_CHEMISTRY 99

static const struct { u8 variant; u8 base; } VARIANTS[] = {
    { CHAR_ID_KOOPA_RED,        CHAR_ID_KOOPA_GREEN    },
    { CHAR_ID_PARATROOPA_GREEN, CHAR_ID_PARATROOPA_RED },
    { CHAR_ID_TOAD_BLUE,        CHAR_ID_TOAD_RED       },
    { CHAR_ID_TOAD_YELLOW,      CHAR_ID_TOAD_RED       },
    { CHAR_ID_TOAD_GREEN,       CHAR_ID_TOAD_RED       },
    { CHAR_ID_TOAD_PURPLE,      CHAR_ID_TOAD_RED       },
    { CHAR_ID_SHYGUY_BLUE,      CHAR_ID_SHYGUY_RED     },
    { CHAR_ID_SHYGUY_YELLOW,    CHAR_ID_SHYGUY_RED     },
    { CHAR_ID_SHYGUY_GREEN,     CHAR_ID_SHYGUY_RED     },
    { CHAR_ID_SHYGUY_BLACK,     CHAR_ID_SHYGUY_RED     },
    { CHAR_ID_PIANTA_RED,       CHAR_ID_PIANTA_BLUE    },
    { CHAR_ID_PIANTA_YELLOW,    CHAR_ID_PIANTA_BLUE    },
    { CHAR_ID_NOKI_RED,         CHAR_ID_NOKI_BLUE      },
    { CHAR_ID_NOKI_GREEN,       CHAR_ID_NOKI_BLUE      },
    { CHAR_ID_BRO_FIRE,         CHAR_ID_BRO_HAMMER     },
    { CHAR_ID_BRO_BOOMERANG,    CHAR_ID_BRO_HAMMER     },
    { CHAR_ID_MAGIKOOPA_RED,    CHAR_ID_MAGIKOOPA_BLUE },
    { CHAR_ID_MAGIKOOPA_GREEN,  CHAR_ID_MAGIKOOPA_BLUE },
    { CHAR_ID_MAGIKOOPA_YELLOW, CHAR_ID_MAGIKOOPA_BLUE },
    { CHAR_ID_DRYBONES_GREEN,   CHAR_ID_DRYBONES_GRAY  },
    { CHAR_ID_DRYBONES_RED,     CHAR_ID_DRYBONES_GRAY  },
    { CHAR_ID_DRYBONES_BLUE,    CHAR_ID_DRYBONES_GRAY  },
};

static int BaseCharacter(int charID)
{
    for (int i = 0; i < (int)LEN(VARIANTS); i++)
        if (VARIANTS[i].variant == charID)
            return VARIANTS[i].base;
    return charID;
}

CGECKO(DuplicatesAndVariantsHaveChemistry,
       .notes = "Characters have chemistry with themselves and with their color variants.");
void DuplicatesAndVariantsHaveChemistry(void)
{
    u8 base[NUM_CHOOSABLE_CHARACTERS];
    for (int id = 0; id < NUM_CHOOSABLE_CHARACTERS; id++)
        base[id] = BaseCharacter(id);

    for (int id = 0; id < NUM_CHOOSABLE_CHARACTERS; id++)
        for (int with = 0; with < NUM_CHOOSABLE_CHARACTERS; with++)
            if (base[id] == base[with])
                StatRowBytes(id)[CHEMISTRY_OFFSET(with)] = FULL_CHEMISTRY;
}
