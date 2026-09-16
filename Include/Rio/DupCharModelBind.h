/*###########################################################
# DupCharModelBind.h -- model binding for duplicate-character rosters
###########################################################*/
// Author: LittleCoaks
// Why the game needs these two hooks: docs/duplicate_characters.md
// Include from a boot code once; two codes with the same name in one build silently drop one.

#ifndef RIO_DUPCHARMODELBIND_H
#define RIO_DUPCHARMODELBIND_H

#include "CGecko/Common.h"
#include "Include/Symbols/dol.h"

#define DUPBIND_SCRATCH_OBJ 0x80370F1C

// No .instruction on either hook: both REPLACE the instruction they overwrite.
CGECKO(DupLoadBindCharID, .address = 0x800156B0);
void DupLoadBindCharID()
{
    READ_GAME_REG(u32, entry, 5);                  // r5 = &table[r9]
    u32 obj = *(u32*)entry;                        // table[r9]
    WRITE_GAME_REG(3, obj ? obj : DUPBIND_SCRATCH_OBJ);
}

CGECKO(DupLoadBindStore, .address = 0x800156E4);
void DupLoadBindStore()
{
    // READ_GAME_REG can't be used twice in one function; saved r<n> is at r30 + 0x8 + (n-3)*4
    register u32 _fp __asm__("r30");
    u32 r3    = *(volatile u32*)(_fp + 0x8);              // saved r3 = table[r9]
    u32 r8    = *(volatile u32*)(_fp + 0x8 + ((8 - 3) << 2)); // saved r8
    u32 model = *(u32*)(hugeAnimStruct_ADDR + (r8 << 2) + 11456);  // model[r8]
    *(u32*)((r3 ? r3 : DUPBIND_SCRATCH_OBJ) + 24) = model;  // bind; scratch on NULL
}

#endif
