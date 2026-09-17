// Byte edits to the master character stat table, written as named fields.
// Layout and the ability-byte split: docs/import_balance.md
#pragma once
#include "Include/static/UnknownHomes_Static.h"

typedef struct { u8 charID; u8 offset; u8 value; } StatEdit;

#define STAT_OFFSET(field)        offsetof(CharacterStats, stats.field)
#define CHEMISTRY_OFFSET(withID)  (offsetof(CharacterStats, chemistry) + (withID))

#define STAT(id, field, value)        { id, STAT_OFFSET(field), value }
#define CHEMISTRY(id, withID, value)  { id, CHEMISTRY_OFFSET(withID), value }
#define ABILITIES_LOW(id, flags)      { id, STAT_OFFSET(FieldingStats) + 3, (flags) & 0xFF }
#define ABILITIES_HIGH(id, flags)     { id, STAT_OFFSET(FieldingStats) + 2, ((flags) >> 8) & 0xFF }

static inline volatile u8* StatRowBytes(int charID)
{
    return (volatile u8*)&Static_Stats_Tables.characterStats[charID];
}

static inline void ApplyStatEdits(const StatEdit* edits, int count)
{
    for (int i = 0; i < count; i++)
        StatRowBytes(edits[i].charID)[edits[i].offset] = edits[i].value;
}

static inline void FillChemistryRow(int charID, u8 value)
{
    volatile u8* row = StatRowBytes(charID) + CHEMISTRY_OFFSET(0);
    for (int with = 0; with < NUM_CHOOSABLE_CHARACTERS; with++)
        row[with] = value;
}

static inline void FillAllChemistry(u8 value)
{
    for (int id = 0; id < NUM_CHOOSABLE_CHARACTERS; id++)
        FillChemistryRow(id, value);
}
