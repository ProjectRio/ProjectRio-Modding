/*###########################################################
# RelTable.h -- the DOL's REL file table and REL loader node
###########################################################*/
// Author: LittleCoaks
// Mechanics: docs/rel_loader.md, docs/debug_rel.md

#ifndef RIO_RELTABLE_H
#define RIO_RELTABLE_H

#include "Include/Symbols/dol.h"
#include "Include/Unknown/File_0x800b0a14.h"

// relFileTable (0x800E8AA8): the menu-slot entry is the first one.
typedef struct RelFileEntry
{
    u32 unk_00;
    u32 flagAndSize;   // flag | decompressed size
    u32 offset;        // in aaaa.dat
    u32 compSize;
} RelFileEntry;

#define relFileTableMenuEntry VAR_ADDRESS(RelFileEntry, relFileTable_ADDR)

#define DEBUG_REL_FLAG_SIZE 0x4005912C
#define DEBUG_REL_OFFSET    0x00150000
#define DEBUG_REL_CSIZE     0x000271C0

// The loader node (relLoaderNode, 0x80111300) has no _ADDR in the decomp yet.
#define RELLOADER_NODE_ADDR 0x80111300
#define RelLoader_Finished  VAR_ADDRESS(s16, RELLOADER_NODE_ADDR + offsetof(DrawingSceneStruct, state))
#define RelLoader_State     VAR_ADDRESS(u16, RELLOADER_NODE_ADDR + offsetof(DrawingSceneStruct, unk_18))

#define RELLOADER_STATE_RELOAD_MENU 0x8
#define RELLOADER_STATE_LOAD_GAME   0xB

static inline void RelTable_PointMenuSlotAtDebugRel(void)
{
    relFileTableMenuEntry.flagAndSize = DEBUG_REL_FLAG_SIZE;
    relFileTableMenuEntry.offset      = DEBUG_REL_OFFSET;
    relFileTableMenuEntry.compSize    = DEBUG_REL_CSIZE;
}

#endif
