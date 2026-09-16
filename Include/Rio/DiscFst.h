/*###########################################################
# DiscFst.h -- is a file on this disc?
###########################################################*/
// Author: LittleCoaks
//
// Fst_ProbePath(path) resolves a disc path through the FST the apploader
// left in RAM: the file's length, or -1 when there is no such file. An FST
// walk only, so it is safe from any hook (no DVD command, never sleeps).

#ifndef DISCFST_H
#define DISCFST_H

#include "CGecko/Common.h"
#include "Include/types.h"
#include "Include/Dolphin/dvd.h"
#include "Include/Dolphin/OS/OSBootInfo.h"

typedef struct FstEntry {
    /* 0x00 */ u8  isDir;
    /* 0x01 */ u8  nameOffset[3];
    /* 0x04 */ u32 offset;                  // directories: parent index
    /* 0x08 */ u32 length;                  // directories: next index; entry 0: entry count
} FstEntry;                                 // size 0xC

#define FST_BOOT_INFO  VAR_ADDRESS(OSBootInfo, 0x80000000)   // the OS boot block

static inline s32 Fst_ProbePath(const char* path)
{
    const FstEntry* fst = (const FstEntry*)FST_BOOT_INFO.FSTLocation;
    s32 e;

    if (fst == 0)
        return -1;
    e = DVDConvertPathToEntrynum((char*)path);
    if (e < 0 || (u32)e >= fst[0].length)
        return -1;
    if (fst[e].isDir != 0)
        return -1;
    return (s32)fst[e].length;
}

#endif /* DISCFST_H */
