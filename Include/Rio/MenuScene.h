/*###########################################################
# MenuScene.h -- build and edit the game's 2D menu scenes from C
###########################################################*/
// Author: LittleCoaks
//
// Every 2D menu in MSSB (main menu, Options, Records, the in-game pause
// screens...) is a set of UI RECORDS in one DOL pool, each naming a LAYOUT
// ELEMENT inside a loaded texture/layout CONTAINER. The game builds a screen
// from a DESCRIPTOR LIST, keeps the records reachable by HANDLE through a
// scene NODE, animates them by stepping a per-record frame counter, and frees
// them by node. This header names those pieces and wraps the DOL routines
// that operate on them, so a mod can:
//
//   * find the records of a stock screen by handle and change their
//     visibility, texture, animation frame or position;
//   * copy a stock record (a button, a label, a bar) and hang the copy in the
//     same tree, one row over -- the game then draws and animates it like
//     the original (RioModPack/Online Menu.c adds an eighth main-menu
//     button this way);
//   * build records of its own from a descriptor list on a node of its own,
//     and free them again (the Online screen's backdrop);
//   * point a container's texture record at pixels shipped in the mod, so a
//     record can show art that is not on the disc;
//   * edit a loaded layout in place: the anchors that place rows, the vertex
//     colours a screen is tinted with;
//   * narrow the UI draw pass to its own records while a custom screen is up.
//
// docs/menu_scenes.md explains the model and walks through each of these.
// Everything here was traced live on the US disc (GYQE01), see that file for
// the method; the DOL addresses are fixed, the menus.rel ones are fixed too
// because the REL always links at the same place.
//
// RULES OF THE ROAD (cgecko payloads):
//   * No mutable payload statics reached from helpers -- cgecko reaches
//     payload data through r31, which is NULL inside a helper. Keep mutable
//     state at claimed RAM addresses (ClaimedFreeMemory.h) and form pointers
//     to read-only tables in the hook body, passing them down as arguments.
//     Every helper here takes what it needs as arguments for that reason.
//   * No floats in hooks unless the hook saves the FPU: positions are written
//     as their IEEE bit patterns (MS_F32 below).
//   * Pixel data the GPU reads in place must be 32-byte aligned
//     (__attribute__((aligned(32))) on the array) and must live in the mod's
//     image, never inside a game texture's area -- those areas belong to
//     other art (a "spare" one turned out to be Toy Field's preview picture).

#ifndef MENUSCENE_H
#define MENUSCENE_H

#include "CGecko/Common.h"
#include "Include/types.h"

// ---- the pool of UI records --------------------------------------------------
// 864 records of 0xC0 bytes at menuGraphicsStructures. A record with flags
// (+0x54) == 0 is free. Fields, as read by maybeProcessUIUpdates (the DOL
// draw pass, 0x80035168) and allocateGraphicsSlot (0x80034F50):
#define MS_POOL          0x8039C3E0
#define MS_POOL_COUNT    864
#define MS_REC_SIZE      0xC0
#define MS_REC(i)        (MS_POOL + (i) * MS_REC_SIZE)
#define MS_REC_INDEX(r)  (((r) - MS_POOL) / MS_REC_SIZE)

#define MS_PARENT(r)     VAR_ADDRESS(u32, (r) + 0x00)   // parent record, 0 = top level
#define MS_NEXT(r)       VAR_ADDRESS(u32, (r) + 0x04)   // next sibling in the parent's child list
#define MS_CHILD(r)      VAR_ADDRESS(u32, (r) + 0x08)   // first child
#define MS_POSX(r)       VAR_ADDRESS(u32, (r) + 0x48)   // f32 bits: offset in the parent's space
#define MS_POSY(r)       VAR_ADDRESS(u32, (r) + 0x4C)
#define MS_POSZ(r)       VAR_ADDRESS(u32, (r) + 0x50)
#define MS_FLAGS(r)      VAR_ADDRESS(u32, (r) + 0x54)   // 0 = free; bit 1 = visible; bit 2 = text record
#define MS_COLOUR(r)     VAR_ADDRESS(u32, (r) + 0x58)   // RGBA multiplier on the layout's vertex colours
#define MS_FRAME(r)      VAR_ADDRESS(u32, (r) + 0x5C)   // animation frame, 16.16 fixed
#define MS_RATE(r)       VAR_ADDRESS(u32, (r) + 0x60)   // frames per frame, 16.16 (normally 0x10000)
#define MS_ELEM(r)       VAR_ADDRESS(u16, (r) + 0x64)   // layout element index in the container
#define MS_SLOT(r)       VAR_ADDRESS(u8,  (r) + 0x66)   // texture slot (the loaded container)
#define MS_LAYER(r)      VAR_ADDRESS(u8,  (r) + 0x67)   // draw layer (from the descriptor)
#define MS_PLAY(r)       VAR_ADDRESS(u8,  (r) + 0x68)   // 0 stopped, 1 count up, 4 count down
#define MS_SUBINDEX(r)   VAR_ADDRESS(s16, (r) + 0x72)   // which of the parent's anchors places this child
#define MS_ANCHOR_MTX(r) ((r) + 0x78)                   // 3x4 matrix the parent's anchor wrote this frame
#define MS_ATTACHED(r)   VAR_ADDRESS(u8,  (r) + 0xA8)   // set when the parent attached it this frame, cleared after drawing
#define MS_OVERRIDE(r,i) VAR_ADDRESS(u16, (r) + 0xAC + (i) * 2)  // per-part texture override, 0xFFFF = none (10 slots)

#define MS_VISIBLE       2
#define MS_NO_OVERRIDE   0xFFFF
#define MS_FRAME_OF(n)   ((u32)(n) << 16)
#define MS_FRAME_NO(r)   (MS_FRAME(r) >> 16)

// Play modes, as the draw pass steps them.
#define MS_PLAY_STOP     0
#define MS_PLAY_FORWARD  1
#define MS_PLAY_BACKWARD 4

// f32 bit patterns for positions without touching the FPU.
#define MS_F32_0         0x00000000
#define MS_F32_NEG(i)    (0x80000000u | MS_F32_POS(i))
// Only small integers are needed in practice; spell them out rather than
// pull in float math: 1..64 in 0x3F800000 + exponent/mantissa form.
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
// A scene NODE is the draw-script item the game built a screen on. Its
// records are numbered by HANDLE (descriptor order) through
// graphicsRelatedArray: entry (node+0x14 + handle) holds the record pointer.
#define MS_GRA_ARRAY     0x80371C30                     // 8 bytes per entry: +0 record, +4 owning node (first only)
#define MS_NODE_BASE(n)  VAR_ADDRESS(u16, (n) + 0x14)   // first entry of this node's handles
#define MS_NODE_COUNT(n) VAR_ADDRESS(u16, (n) + 0x16)   // how many
#define MS_CURRENT_NODE  VAR_ADDRESS(u32, 0x803CC1B8)   // currentDrawingItem: the node whose function is running

static inline u32 MS_Record(u32 node, int handle)
{
    return VAR_ADDRESS(u32, MS_GRA_ARRAY + (MS_NODE_BASE(node) + handle) * 8);
}

// ---- the DOL routines --------------------------------------------------------
typedef void* (*ms_memcpy_t)(void*, const void*, int);
typedef void  (*ms_flush_t)(const void*, u32);
typedef void  (*ms_scene_t)(u32 node, const void* descriptors);
typedef void  (*ms_node_t)(u32 node);
typedef void  (*ms_loadicon_t)(u32 node, int handle, int part, int table, int idx);
typedef void  (*ms_process_t)(int channel, int code);
typedef void  (*ms_lock_t)(int channel);

#define MS_memcpy                 ((ms_memcpy_t)0x800054F4)
#define MS_DCFlushRange           ((ms_flush_t)0x8006E894)
#define MS_addGraphicsElementToScene      ((ms_scene_t)0x80034E20)   // build records from a descriptor list onto a node
#define MS_removeGraphicsElementFromScene ((ms_node_t)0x80034CEC)    // free every record of a node (by handle; never walks children)
#define MS_load_Icon              ((ms_loadicon_t)0x800363D8)  // set a record's part texture from the container's icon table
#define MS_updateProcessCode      ((ms_process_t)0x800625A4)  // updateCharacterSelectProcessCode: post a menu process code
#define MS_makeCursorUnmovable    ((ms_lock_t)0x800626EC)     // block menu input (counted)
#define MS_makeCursorMovable      ((ms_lock_t)0x80062674)     // ...and release it (also zeroes the process code)

// ---- descriptors ---------------------------------------------------------------
// One 0x20-byte record descriptor, as allocateGraphicsSlot reads it. A list
// is an array of these ending in one whose type is 3. Parent is a handle in
// the same list, or 0xFF for a top-level record. Mode 0 = visible and
// stopped, 1 = visible and playing, 2/3 = created hidden.
#define MS_DESC(elem, mode, layer, parent, tag, sub)                          \
    0x00, 0x00, (u8)((elem) >> 8), (u8)(elem),                                  \
    0, 0, 0, 0,  0, 0, 0, 0,  0xFF, 0xFF, 0xFF, 0xFF,                            \
    (mode), (layer), (u8)((parent) >> 8), (u8)(parent), (tag), 0, 0, 0,          \
    0, 0, 0, 0,  0x00, 0x01, (u8)((sub) >> 8), (u8)(sub)
#define MS_DESC_END                                                           \
    0x00, 0x03, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                         \
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define MS_DESC_SIZE     0x20
#define MS_NO_PARENT     0xFF

/* Build a descriptor list's records on `node` (24 bytes of claimed RAM,
 * zeroed here). Returns the count made. Free them with MS_RemoveScene. */
static inline int MS_BuildScene(u32 node, const u8* descriptors)
{
    int i;
    for (i = 0; i < 24; i += 4)
        VAR_ADDRESS(u32, node + i) = 0;
    MS_addGraphicsElementToScene(node, descriptors);
    return MS_NODE_COUNT(node);
}

static inline void MS_RemoveScene(u32 node)
{
    MS_removeGraphicsElementFromScene(node);
}

/* The lowest and highest pool index among a node's records, for
 * MS_NarrowDraw. Returns 0 if the node has none. */
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
// A loaded container occupies a texture SLOT (0x3C bytes): +0x30 the buffer,
// +0x34 its texture header (u16 count, then 0x20-byte records), +0x38 its
// layout. Slots are found by the TAG the screen loaded the container under.
#define MS_SLOT_TAGS     0x8023D6C0                     // s16 tag per slot, -1 = free
#define MS_SLOTS         0x803C4BE0
#define MS_SLOT_COUNT    0x14
#define MS_SLOT_SIZE     0x3C

static inline int MS_SlotOfTag(int tag)
{
    int s;
    for (s = 0; s < MS_SLOT_COUNT; s++)
        if (VAR_ADDRESS(s16, MS_SLOT_TAGS + s * 2) == tag)
            return s;
    return -1;
}
static inline u32 MS_TextureHeaderOfTag(int tag)
{
    int s = MS_SlotOfTag(tag);
    return s < 0 ? 0 : VAR_ADDRESS(u32, MS_SLOTS + s * MS_SLOT_SIZE + 0x34);
}
static inline u32 MS_LayoutOfTag(int tag)
{
    int s = MS_SlotOfTag(tag);
    return s < 0 ? 0 : VAR_ADDRESS(u32, MS_SLOTS + s * MS_SLOT_SIZE + 0x38);
}

// Texture records (0x20 bytes each, right after the u16 count). Pointers are
// already relocated in RAM.
#define MS_TEX_COUNT(hdr)      VAR_ADDRESS(u16, (hdr))
#define MS_TEX(hdr, i)         ((hdr) + (i) * 0x20)
#define MS_TEX_INDEX(t)        VAR_ADDRESS(u16, (t) + 0x00)
#define MS_TEX_DATA(t)         VAR_ADDRESS(u32, (t) + 0x04)   // pixel data pointer
#define MS_TEX_TLUT(t)         VAR_ADDRESS(u32, (t) + 0x08)   // palette pointer (palette formats)
#define MS_TEX_H(t)            VAR_ADDRESS(u16, (t) + 0x0C)
#define MS_TEX_W(t)            VAR_ADDRESS(u16, (t) + 0x0E)
#define MS_TEX_FMT(t)          VAR_ADDRESS(u8,  (t) + 0x1B)   // GX texture format (8 = C4, 9 = C8, ...)
#define MS_TEX_TLUT_N(t)       VAR_ADDRESS(u16, (t) + 0x1C)
#define MS_TEX_TLUT_FMT(t)     VAR_ADDRESS(u8,  (t) + 0x1E)

/* Make texture record `victim` describe the same kind of image as record
 * `model` (size, format, palette format, mips, lod) but with pixels and
 * palette from `data`/`tlut` -- 32-byte-aligned arrays in the mod's image.
 * Idempotent, so it can be redone every time the container is reloaded. */
static inline void MS_RetargetTexture(u32 hdr, int victim, int model, const void* data, const void* tlut)
{
    u32 dst = MS_TEX(hdr, victim), src = MS_TEX(hdr, model);
    u16 index = MS_TEX_INDEX(dst);
    MS_memcpy((void*)dst, (void*)src, 0x20);
    MS_TEX_INDEX(dst) = index;
    MS_TEX_DATA(dst)  = (u32)data;
    MS_TEX_TLUT(dst)  = (u32)tlut;
}

// Layouts: +8 -> element table {u32 count|flags; u32; u32 ptr[count]};
// element -> {u32 nparts|flags; u32; u32 part[nparts]}; part -> {u16 count,
// u16 size, sub-records...}. Sub-record contents are element specific;
// anchors (which place child records) keep x at +0x14 and y at +0x16 of the
// first sub-record, and quad colours are RGBA words.
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

/* Copy `src` into the first free record as the first child of `parent`,
 * with no children of its own. Returns the copy, or 0 if the pool is full.
 * The copy keeps src's element, sub-index, textures and animation state, so
 * the parent's anchor places it exactly where src is; move it with MS_POSX/Y
 * (in the parent's space) afterwards. */
static inline u32 MS_CopyRecord(u32 src, u32 parent)
{
    u32 dst = MS_FirstFreeRecord();
    if (dst == 0)
        return 0;
    MS_memcpy((void*)dst, (void*)src, MS_REC_SIZE);
    MS_CHILD(dst)    = 0;
    MS_ATTACHED(dst) = 0;
    MS_PARENT(dst)   = parent;
    MS_NEXT(dst)     = parent ? MS_CHILD(parent) : 0;
    if (parent)
        MS_CHILD(parent) = dst;
    return dst;
}

/* Take a record out of its parent's child list (if the parent is still in
 * use) and free it. Free children before parents. */
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

/* Start a record's timeline at `frame` counting in `mode`. */
static inline void MS_Play(u32 rec, u32 frame, u8 mode)
{
    MS_FRAME(rec) = MS_FRAME_OF(frame);
    MS_PLAY(rec)  = mode;
}
/* Park a record on `frame`. */
static inline void MS_Hold(u32 rec, u32 frame)
{
    MS_FRAME(rec) = MS_FRAME_OF(frame);
    MS_PLAY(rec)  = MS_PLAY_STOP;
}
/* Stop a playing record once it reaches `end`; returns 1 when it is stopped
 * there. Counting up, "reaches" means >= (one frame can pass between a move
 * and its first check); counting down, <=. Timelines that hold on their
 * last frame make an exact check hang forever. */
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
// maybeProcessUIUpdates draws pool records [start, end) each frame, then the
// text pass. Narrowing the pair to a scene's own records hides everything
// else without touching it; the Options Menu blanks its screen by setting
// end to 0. Always restore -- a watchdog that restores when the screen is no
// longer current is the pattern (Options Menu.c, Online Menu.c).
#define MS_DRAW_START    VAR_ADDRESS(u32, 0x803CBC98)
#define MS_DRAW_END      VAR_ADDRESS(u32, 0x803CB814)
#define MS_DRAW_END_MAX  0x360

static inline void MS_NarrowDraw(u32 lo, u32 hi, u32* savedStart, u32* savedEnd)
{
    *savedStart = MS_DRAW_START;
    *savedEnd   = MS_DRAW_END;
    MS_DRAW_START = lo;
    MS_DRAW_END   = hi + 1;
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
// 0x803CBBCC -> {u16 rel, u16 screenCode, u16 menuProcess, u16 prevScreen,
// u16 prevProcess}; changeScreenVariables sets prev = current, current = new,
// process = 0.
#define MS_MENU_CTRL         VAR_ADDRESS(u32, 0x803CBBCC)
#define MS_SCREEN_CODE       VAR_ADDRESS(u16, MS_MENU_CTRL + 2)
#define MS_MENU_PROCESS      VAR_ADDRESS(u16, MS_MENU_CTRL + 4)
#define MS_PREV_SCREEN       VAR_ADDRESS(u16, MS_MENU_CTRL + 6)
#define MS_PREV_PROCESS      VAR_ADDRESS(u16, MS_MENU_CTRL + 8)
#define MS_SCREEN_MAIN_MENU  5
#define MS_SCREEN_OPTIONS    6
#define MS_changeScreenVariables ((int (*)(int))0x80640234)

static inline int MS_MenuCtrlValid(void)
{
    u32 c = MS_MENU_CTRL;
    return c >= 0x80000000 && c < 0x81800000;
}

/* Go back to the main menu the way an Options exit does. mainMenuScreen only
 * RESUMES (no reload) when prevScreen is 6, 9 or 12; a custom screenCode
 * there would send it down the cold path against containers that are still
 * loaded, so the block is written directly instead of calling
 * changeScreenVariables. */
static inline void MS_ReturnToMainMenu(void)
{
    MS_PREV_PROCESS = MS_MENU_PROCESS;
    MS_PREV_SCREEN  = MS_SCREEN_OPTIONS;
    MS_SCREEN_CODE  = MS_SCREEN_MAIN_MENU;
    MS_MENU_PROCESS = 0;
}

#endif /* MENUSCENE_H */
