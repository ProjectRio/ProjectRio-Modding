/*###########################################################
# MenuScene.h -- build and edit the game's 2D menu scenes from C
###########################################################*/
// Author: LittleCoaks
//
// Names the UI record pool, scene nodes/handles, containers and layouts, and
// wraps the DOL routines on them: MS_Record(node, handle) to reach a stock
// record, MS_CopyRecord/MS_DropRecord to clone one, MS_BuildScene/MS_RemoveScene
// for records of your own, MS_RetargetTexture, MS_NarrowDraw/MS_RestoreDraw,
// MS_ScreenWatchdog for the MSSB_ALWAYS safety net.
// Model, offsets and the cgecko rules of the road: docs/menu_scenes.md.

#ifndef MENUSCENE_H
#define MENUSCENE_H

#include "CGecko/Common.h"
#include "Include/types.h"
#include "Include/Dolphin/stl.h"
#include "Include/Symbols/dol.h"
#include "Include/menus/yd_step.h"
#include "Include/Unknown/File_0x80034cec.h"
#include "Include/Unknown/File_0x80034e20.h"
#include "Include/Unknown/File_0x80062674.h"
#include "Include/Unknown/File_0x800b0a14.h"

// ---- the pool of UI records --------------------------------------------------
// Records are handled as u32 addresses; the fields come from the decomp's UIRecord.
#define MS_POOL          0x8039C3E0                     // menuGraphicsStructures (unbound extern)
#define MS_POOL_COUNT    UI_RECORD_COUNT
#define MS_REC_SIZE      sizeof(UIRecord)
#define MS_REC(i)        (MS_POOL + (i) * MS_REC_SIZE)
#define MS_REC_INDEX(r)  (((r) - MS_POOL) / MS_REC_SIZE)
#define MS_REC_PTR(r)    ((UIRecord*)(r))
#define MS_REC_U32(r, f) VAR_ADDRESS(u32, (u32)&MS_REC_PTR(r)->f)

#define MS_PARENT(r)     MS_REC_U32(r, parent)
#define MS_NEXT(r)       MS_REC_U32(r, next)
#define MS_CHILD(r)      MS_REC_U32(r, firstChild)
#define MS_POSX(r)       MS_REC_U32(r, pos.x)           // f32 bits
#define MS_POSY(r)       MS_REC_U32(r, pos.y)
#define MS_POSZ(r)       MS_REC_U32(r, pos.z)
#define MS_FLAGS(r)      (MS_REC_PTR(r)->flags)         // 0 = free
#define MS_COLOUR(r)     (MS_REC_PTR(r)->rgba)
#define MS_FRAME(r)      (MS_REC_PTR(r)->frame)         // 16.16
#define MS_RATE(r)       (MS_REC_PTR(r)->rate)
#define MS_ELEM(r)       (MS_REC_PTR(r)->elementIndex)
#define MS_SLOT(r)       (MS_REC_PTR(r)->textureSlot)
#define MS_LAYER(r)      (MS_REC_PTR(r)->layer)
#define MS_PLAY(r)       (MS_REC_PTR(r)->playMode)
#define MS_SUBINDEX(r)   (MS_REC_PTR(r)->anchorSub)
#define MS_ANCHOR_MTX(r) ((u32)&MS_REC_PTR(r)->anchor)
#define MS_ATTACHED(r)   (MS_REC_PTR(r)->attachedThisFrame)
#define MS_OVERRIDE(r,i) (MS_REC_PTR(r)->textureOverride[i])

#define MS_VISIBLE       UI_FLAG_VISIBLE
#define MS_NO_OVERRIDE   UI_NO_OVERRIDE
#define MS_FRAME_OF(n)   ((u32)(n) << 16)
#define MS_FRAME_NO(r)   (MS_FRAME(r) >> 16)

#define MS_PLAY_STOP     UI_PLAY_STOP
#define MS_PLAY_FORWARD  UI_PLAY_FORWARD
#define MS_PLAY_BACKWARD UI_PLAY_BACKWARD

// f32 bit patterns for positions without touching the FPU.
#define MS_F32_0         0x00000000
#define MS_F32_NEG(i)    (0x80000000u | MS_F32_POS(i))
#define MS_F32_POS(i)    ms_f32_of_int(i)

static inline u32 ms_f32_of_int(u32 i)
{
    u32 e = 0, m;
    if (i == 0) return 0;
    m = i;
    while (m >= 2) { m >>= 1; e++; }              // e = floor(log2 i)
    return ((127 + e) << 23) | ((i << (23 - e)) & 0x7FFFFF);
}

// ---- scene nodes and handles ---------------------------------------------------
#define MS_GRA_ARRAY     0x80371C30                     // graphicsRelatedArray (unbound)
#define MS_NODE_BASE(n)  VAR_ADDRESS(u16, (u32)&((DrawingSceneStruct*)(n))->unk_14)       // firstHandle
#define MS_NODE_COUNT(n) VAR_ADDRESS(u16, (u32)&((DrawingSceneStruct*)(n))->unk_14 + 2)   // handleCount
#define MS_CURRENT_NODE  ((u32)currentDrawingItem)

static inline u32 MS_Record(u32 node, int handle)
{
    return VAR_ADDRESS(u32, MS_GRA_ARRAY + (MS_NODE_BASE(node) + handle) * 8);
}

// ---- descriptors ---------------------------------------------------------------
#define MS_DESC(elem, mode, layer, parent, tag, sub)                          \
    0x00, 0x00, (u8)((elem) >> 8), (u8)(elem),                                  \
    0, 0, 0, 0,  0, 0, 0, 0,  0xFF, 0xFF, 0xFF, 0xFF,                            \
    (mode), (layer), (u8)((parent) >> 8), (u8)(parent), (tag), 0, 0, 0,          \
    0, 0, 0, 0,  0x00, 0x01, (u8)((sub) >> 8), (u8)(sub)
#define MS_DESC_END                                                           \
    0x00, 0x03, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                         \
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define MS_DESC_SIZE     sizeof(UIRecordDescriptor)
#define MS_NO_PARENT     UI_NO_PARENT

/* Build a descriptor list's records on `node` (24 bytes of claimed RAM,
 * zeroed here). Returns the count made. */
static inline int MS_BuildScene(u32 node, const u8* descriptors)
{
    int i;
    for (i = 0; i < 24; i += 4)
        VAR_ADDRESS(u32, node + i) = 0;
    addGraphicsElementToScene((DrawingSceneStruct*)node, (const UIRecordDescriptor*)descriptors);
    return MS_NODE_COUNT(node);
}

static inline void MS_RemoveScene(u32 node)
{
    removeGraphicsElementFromScene((DrawingSceneStruct*)node);
}

/* Lowest and highest pool index among a node's records; 0 if it has none. */
static inline int MS_SceneIndexRange(u32 node, u32* lo, u32* hi)
{
    int h, n = MS_NODE_COUNT(node);
    u32 r, a = 0xFFFFFFFF, b = 0;
    for (h = 0; h < n; h++)
    {
        r = MS_Record(node, h);
        if (r < a) a = r;
        if (r > b) b = r;
    }
    if (!n) return 0;
    *lo = MS_REC_INDEX(a);
    *hi = MS_REC_INDEX(b);
    return 1;
}

// ---- containers: texture slots, texture records, layouts -------------------------
#define MS_SLOT_TAGS     ARRAY_1D_ADDRESS(s16, TEXTURE_SLOT_COUNT, 0x8023D6C0)                  // textureContainerTags (unbound)
#define MS_SLOTS         ARRAY_1D_ADDRESS(TextureContainerSlot, TEXTURE_SLOT_COUNT, 0x803C4BE0) // textureContainerSlots (unbound)
#define MS_SLOT_COUNT    TEXTURE_SLOT_COUNT

static inline int MS_SlotOfTag(int tag)
{
    int s;
    for (s = 0; s < MS_SLOT_COUNT; s++)
        if (MS_SLOT_TAGS[s] == tag)
            return s;
    return -1;
}
static inline u32 MS_TextureHeaderOfTag(int tag)
{
    int s = MS_SlotOfTag(tag);
    return s < 0 ? 0 : (u32)MS_SLOTS[s].textures;
}
static inline u32 MS_LayoutOfTag(int tag)
{
    int s = MS_SlotOfTag(tag);
    return s < 0 ? 0 : (u32)MS_SLOTS[s].layout;
}

#define MS_TEX_COUNT(hdr)      (((TextureHeader*)(hdr))->count)
#define MS_TEX(hdr, i)         ((hdr) + (i) * sizeof(TextureRecord))
#define MS_TEXREC(t)           ((TextureRecord*)(t))
#define MS_TEX_INDEX(t)        (MS_TEXREC(t)->index)
#define MS_TEX_DATA(t)         (MS_TEXREC(t)->pixels)
#define MS_TEX_TLUT(t)         (MS_TEXREC(t)->tlut)
#define MS_TEX_H(t)            (MS_TEXREC(t)->height)
#define MS_TEX_W(t)            (MS_TEXREC(t)->width)
#define MS_TEX_FMT(t)          (MS_TEXREC(t)->gxFormat)
#define MS_TEX_TLUT_N(t)       (MS_TEXREC(t)->tlutEntries)
#define MS_TEX_TLUT_FMT(t)     (MS_TEXREC(t)->tlutFormat)

/* Make texture record `victim` describe the same kind of image as `model`
 * but with pixels/palette from `data`/`tlut` (32-byte-aligned, in the mod's
 * image). Idempotent. */
static inline void MS_RetargetTexture(u32 hdr, int victim, int model, const void* data, const void* tlut)
{
    u32 dst = MS_TEX(hdr, victim), src = MS_TEX(hdr, model);
    u16 index = MS_TEX_INDEX(dst);
    memcpy((void*)dst, (void*)src, sizeof(TextureRecord));
    MS_TEX_INDEX(dst) = index;
    MS_TEX_DATA(dst)  = (void*)data;
    MS_TEX_TLUT(dst)  = (void*)tlut;
}

#define MS_LAYOUT_ELEMS(l)     VAR_ADDRESS(u32, (l) + 8)
#define MS_ELEM_COUNT(l)       (VAR_ADDRESS(u32, MS_LAYOUT_ELEMS(l)) & 0x7FFFFFFF)
#define MS_LAYOUT_ELEM(l, e)   VAR_ADDRESS(u32, MS_LAYOUT_ELEMS(l) + 8 + (e) * 4)
#define MS_ELEM_NPARTS(ep)     (VAR_ADDRESS(u32, (ep)) & 0x7FFFFFFF)
#define MS_ELEM_PART(ep, j)    VAR_ADDRESS(u32, (ep) + 8 + (j) * 4)
#define MS_PART_COUNT(pp)      VAR_ADDRESS(u16, (pp) + 0)
#define MS_PART_SUBSIZE(pp)    VAR_ADDRESS(u16, (pp) + 2)
#define MS_PART_ANCHOR_X(pp)   VAR_ADDRESS(u16, (pp) + 4 + 0x14)
#define MS_PART_ANCHOR_Y(pp)   VAR_ADDRESS(u16, (pp) + 4 + 0x16)

/* Swap colour words in every part of one element: from[k] -> to[k]. Call
 * again with the arrays swapped to put the layout back. */
static inline void MS_RecolourElement(u32 layout, int elem, const u32* from, const u32* to, int n)
{
    u32 ep, part, off, len;
    int nparts, j, k;

    if (MS_ELEM_COUNT(layout) <= (u32)elem)
        return;
    ep = MS_LAYOUT_ELEM(layout, elem);
    nparts = MS_ELEM_NPARTS(ep);
    for (j = 0; j < nparts; j++)
    {
        part = MS_ELEM_PART(ep, j);
        len  = 4 + MS_PART_COUNT(part) * MS_PART_SUBSIZE(part);
        if (len < 8 || len > 0x400)
            continue;
        for (off = 4; off + 4 <= len; off += 4)
            for (k = 0; k < n; k++)
                if (VAR_ADDRESS(u32, part + off) == from[k])
                {
                    VAR_ADDRESS(u32, part + off) = to[k];
                    break;
                }
    }
}

// ---- records: copy, unlink, free -------------------------------------------
static inline u32 MS_FirstFreeRecord(void)
{
    int i;
    for (i = 0; i < MS_POOL_COUNT; i++)
        if (MS_FLAGS(MS_REC(i)) == 0)
            return MS_REC(i);
    return 0;
}

/* Copy `src` into the first free record as the first child of `parent`, with
 * no children of its own. Returns the copy, or 0 if the pool is full. */
static inline u32 MS_CopyRecord(u32 src, u32 parent)
{
    u32 dst = MS_FirstFreeRecord();
    if (dst == 0)
        return 0;
    memcpy((void*)dst, (void*)src, MS_REC_SIZE);
    MS_CHILD(dst)    = 0;
    MS_ATTACHED(dst) = 0;
    MS_PARENT(dst)   = parent;
    MS_NEXT(dst)     = parent ? MS_CHILD(parent) : 0;
    if (parent)
        MS_CHILD(parent) = dst;
    return dst;
}

/* Unlink from a still-live parent and free. Free children before parents. */
static inline void MS_DropRecord(u32 rec)
{
    u32 parent, p;
    if (rec == 0)
        return;
    parent = MS_PARENT(rec);
    if (parent != 0 && MS_FLAGS(parent) != 0)
    {
        if (MS_CHILD(parent) == rec)
            MS_CHILD(parent) = MS_NEXT(rec);
        else
            for (p = MS_CHILD(parent); p != 0; p = MS_NEXT(p))
                if (MS_NEXT(p) == rec)
                {
                    MS_NEXT(p) = MS_NEXT(rec);
                    break;
                }
    }
    MS_FLAGS(rec)  = 0;
    MS_PARENT(rec) = 0;
    MS_NEXT(rec)   = 0;
    MS_CHILD(rec)  = 0;
}

// ---- animation ---------------------------------------------------------------
static inline void MS_Show(u32 rec)  { MS_FLAGS(rec) |=  MS_VISIBLE; }
static inline void MS_Hide(u32 rec)  { MS_FLAGS(rec) &= ~MS_VISIBLE; }

static inline void MS_Play(u32 rec, u32 frame, u8 mode)
{
    MS_FRAME(rec) = MS_FRAME_OF(frame);
    MS_PLAY(rec)  = mode;
}
static inline void MS_Hold(u32 rec, u32 frame)
{
    MS_FRAME(rec) = MS_FRAME_OF(frame);
    MS_PLAY(rec)  = MS_PLAY_STOP;
}
/* Stop a playing record once it reaches `end`; returns 1 when stopped there.
 * "Reaches" is >= / <=: timelines that hold on their last frame make an
 * exact check hang forever. */
static inline int MS_StopAt(u32 rec, u32 end)
{
    u32 now = MS_FRAME_NO(rec);
    if (MS_PLAY(rec) == MS_PLAY_STOP)
        return now == end;
    if ((MS_PLAY(rec) == MS_PLAY_BACKWARD) ? (now <= end) : (now >= end))
    {
        MS_Hold(rec, end);
        return 1;
    }
    return 0;
}

// ---- the UI draw pass ------------------------------------------------------------
// Always restore; a watchdog that restores when the screen is no longer
// current is the pattern (Options Menu.c, Online Menu.c).
#define MS_DRAW_START    VAR_ADDRESS(u32, 0x803CBC98)   // uiDrawLoopStart (unbound extern)
#define MS_DRAW_END      VAR_ADDRESS(u32, 0x803CB814)   // uiDrawLoopEnd
#define MS_DRAW_END_MAX  UI_RECORD_COUNT

static inline void MS_NarrowDraw(u32 lo, u32 hi, u32* savedStart, u32* savedEnd)
{
    *savedStart = MS_DRAW_START;
    *savedEnd   = MS_DRAW_END;
    MS_DRAW_START = lo;
    MS_DRAW_END   = hi + 1;
}
/* Draw no records at all; the text pass still runs. */
static inline void MS_BlankDraw(u32* savedStart, u32* savedEnd)
{
    MS_NarrowDraw(0, (u32)-1, savedStart, savedEnd);
}
static inline void MS_RestoreDraw(u32 savedStart, u32 savedEnd)
{
    if (savedEnd != 0 && savedEnd <= MS_DRAW_END_MAX)
    {
        MS_DRAW_START = savedStart;
        MS_DRAW_END   = savedEnd;
    }
}

// ---- the menu control block -----------------------------------------------------
#define MS_MENU_CTRL         VAR_ADDRESS(menuControlStruct*, menuControlVariables_ADDR)
#define MS_SCREEN_CODE       (MS_MENU_CTRL->currentScreen)
#define MS_MENU_PROCESS      (MS_MENU_CTRL->currentState)    // zeroed on every screen change
#define MS_PREV_SCREEN       (MS_MENU_CTRL->previousScreen)
#define MS_PREV_PROCESS      (MS_MENU_CTRL->previousState)
#define MS_SCREEN_MAIN_MENU  5
#define MS_SCREEN_OPTIONS    6

static inline int MS_MenuCtrlValid(void)
{
    u32 c = (u32)MS_MENU_CTRL;
    return c >= 0x80000000 && c < 0x81800000;
}

/* 1 when `screenCode` is no longer the current screen (or there is no menu
 * control block): the scene was left by a route its own code never saw. */
static inline int MS_ScreenLeft(u16 screenCode)
{
    return !MS_MenuCtrlValid() || MS_SCREEN_CODE != screenCode;
}

/* The MSSB_ALWAYS safety net: run `onExit` once the screen has been left.
 * Returns 1 when it fired. */
static inline int MS_ScreenWatchdog(u16 screenCode, void (*onExit)(void))
{
    if (!MS_ScreenLeft(screenCode))
        return 0;
    onExit();
    return 1;
}

/* Go back to the main menu the way an Options exit does: mainMenuScreen only
 * RESUMES (no reload) when prevScreen is 6, 9 or 12. */
static inline void MS_ReturnToMainMenu(void)
{
    MS_PREV_PROCESS = MS_MENU_PROCESS;
    MS_PREV_SCREEN  = MS_SCREEN_OPTIONS;
    MS_SCREEN_CODE  = MS_SCREEN_MAIN_MENU;
    MS_MENU_PROCESS = 0;
}

#endif /* MENUSCENE_H */
