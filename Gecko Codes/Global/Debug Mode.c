/*###########################################################
# Debug Mode   (loads debug.rel over the menu REL)
###########################################################*/
// Author: LittleCoaks
// Boots into the game's unused developer debug menu and draws its missing
// text. Everything about the module, the loader, and the fixes below is in
// docs/debug_rel.md.

#include "Include/text/text_channel.h"
#include "Include/musyx/musyx.h"
#include "Include/Unknown/File_0x800b0a14.h"
#include "Include/Dolphin/OS/OSCache.h"
#include "Include/Rio/RelTable.h"
#include "Include/Rio/PatchTable.h"

// claimed RAM, see ClaimedFreeMemory.h
#define TEXT_SLOTS      17            // rowsPerPage (16) + a cursor marker; fewer makes the cursor walk off the page
#define TEXT_MAXLEN     27
#define VALUE_COLUMN    13            // glyph column the value starts at
#define TEXT_FIRST      (30 - TEXT_SLOTS)
#define TEXT_BUF_ADDR   0x802EC504    // TEXT_SLOTS * (TEXT_MAXLEN+1) u16 = 952 B
#define g_menuState     VAR_ADDRESS(u32, 0x802EC8BC)  // captured fn_80048BEC arg

#define g_loadedModuleId    VAR_ADDRESS(u32, 0x8063EFC0)   // 1 = debug.rel
// bank heads are two DrawingSceneStructs apart in DSS_Head1
#define DRAW_BANK_NODE(idx) ((u32)&DSS_Head1[0] + (u32)(idx) * 0x80)
#define DEBUG_SELECTOR_FN   0x80640734

// entry 6, the Sound Test (rep_02A8)
#define SND_TOP_FN         0x806493DC   // fn_1_A348 (.text 0xA348)
#define SND_LABELS         0x80672298   // 19 const char*; [0..4] are the items
#define SND_CURSOR         0x806E1B98   // lbl_1_common_bss_49A78, s8, 0..4
#define SND_ITEMS          5
#define SND_VOICE_FN       0x8064B044   // fn_1_BFB0  (.text 0xBFB0)
#define SND_VOICE_NO       0x8069B176   // lbl_1_bss_3056, s16, wraps at 0xE9
#define SND_VOICE2_FN      0x8064AC94   // fn_1_BC00  (.text 0xBC00)
#define SND_VOICE2_NO      0x8069B174   // lbl_1_bss_3054, s16
#define SND_CAPTION        (SND_LABELS + 5 * 4)
#define SND_TRAIN_FN       0x806494F8   // fn_1_A464  (.text 0xA464)
#define SND_TRAIN_NO       0x806727E4   // lbl_1_data_1CA4, s16
#define SND_SE_FN          0x8064A64C   // fn_1_B5B8  (.text 0xB5B8)
#define SND_SE_MODE        0x8069B100   // lbl_1_bss_2FD8 +0x08, u8: Y->0, X->1
#define SND_SE_ROW         0x8069B101   // +0x09, u8 0..3
#define SND_SE_V0          0x806727E2   // lbl_1_data_FC0 +0xCE2, s16 (wraps 0x151..0x1B6)
#define SND_SE_V1          0x8069B102   // +0x0A
#define SND_SE_V2          0x8069B104   // +0x0C
#define SND_SE_V3          0x8069B106   // +0x0E
#define SND_SE_TBL1        0x806723CC   // lbl_1_data_FC0+0x8CC, 48 ids (idx 0..0x2F)
#define SND_SE_TBL2        0x8067242C   // lbl_1_data_FC0+0x92C, 24 ids (idx 0..0x17)

// entry 0, the Stadium Viewer's hidden picker (rep_0138)
#define STADIUM_PICK_FN    0x806458DC
#define STADIUM_SEL        0x806DF604   // .bss +0x474E4, u8 0..6, up = -1
#define STADIUM_COUNT      7

#define TXT_WHITE  0xFFFFFFFF
#define TXT_YELLOW 0xFFFF20FF
#define TXT_GRAY   0x909090FF

// ASCII -> MSSB glyph code (same table as ScreenText.h)
static u16 DebugGlyph(char c)
{
    if (c == ' ')             return 0x4002;
    if (c >= 'a' && c <= 'z') return c - 'a' + 62;
    if (c == ']')             return 59;
    if (c == '^')             return 60;
    if (c >= '!' && c <= '[') return c - '!';
    return 30;                /* '?' */
}

// Fill one pool block with a literal string, optionally followed by a value
// column (fixedPoint = scaled by 100000, printed as n.nn).
static void DebugRow(int slot, int x, int y, u32 color, const char* s,
                     const s32* valPtr, s32 fixedPoint)
{
    u16* out = (u16*)(TEXT_BUF_ADDR + slot * ((TEXT_MAXLEN + 1) * 2));
    ScreenText* t = &screenTextArray.blocks[TEXT_FIRST + slot];
    u32* raw = (u32*)t;
    int n = 0;
    int i;

    while (*s != 0 && n < TEXT_MAXLEN)
        out[n++] = DebugGlyph(*s++);

    if (valPtr != 0)
    {
        s32 v = *valPtr;
        s32 whole = fixedPoint ? v / 100000 : v;
        s32 frac = fixedPoint ? v % 100000 : 0;
        u32 u;
        u16 tmp[12];
        int c = 0;

        while (n < VALUE_COLUMN && n < TEXT_MAXLEN)   /* pad to the value column */
            out[n++] = 0x4002;                        /* space */
        if (v < 0 && n < TEXT_MAXLEN)
            out[n++] = 12;                            /* '-' */
        if (whole < 0)
            whole = -whole;
        if (frac < 0)
            frac = -frac;
        u = (u32)whole;
        do
        {
            tmp[c++] = (u16)(15 + (u % 10));          /* '0'..'9' = 15..24 */
            u = u / 10;
        } while (u != 0 && c < 10);
        while (c > 0 && n < TEXT_MAXLEN)
            out[n++] = tmp[--c];
        if (fixedPoint)
        {
            u32 f = (u32)frac / 1000;                 /* two decimals */
            if (n < TEXT_MAXLEN)
                out[n++] = 13;                        /* '.' */
            if (n < TEXT_MAXLEN)
                out[n++] = (u16)(15 + ((f / 10) % 10));
            if (n < TEXT_MAXLEN)
                out[n++] = (u16)(15 + (f % 10));
        }
    }

    out[n] = 0x4000;                 /* end of string */

    for (i = 0; i < 14; i++)         /* 14 words = the whole block */
        raw[i] = 0;
    t->bankText             = (u16*)out;
    t->color                       = (s32)color;
    t->x                        = (u16)x;
    t->y                        = (u16)y;
    t->maxLettersToDraw        = -1;   /* -1 = draw the whole string */
    t->drawGroup                = 5;    /* draw group; 1-8 render every frame */
    t->style                  = 1;    /* small font */
    t->lineSpacing                = 2;
    t->justify = 0;    /* left */
    t->state                = 2;    /* state: active -- set last */
}

static void DebugLine(int slot, int x, int y, u32 color, const char* s)
{
    DebugRow(slot, x, y, color, s, 0, 0);
}

// Draw a NUL-separated, double-NUL-terminated list of lines, highlighting the
// selected one and putting a cursor beside it. Returns slots consumed.
static int DebugList(int slot, int x, int y, int step, const char* list, s32 sel)
{
    int i;

    for (i = 0; *list != 0; i++)
    {
        DebugRow(slot + i, x, y + i * step,
                 (i == sel) ? TXT_YELLOW : TXT_WHITE, list, 0, 0);
        while (*list != 0)
            list++;
        list++;                       /* step over the NUL to the next entry */
    }
    if (sel >= 0 && sel < i)
        DebugRow(slot + i, x - 16, y + sel * step, TXT_YELLOW, ">", 0, 0);
    return i + 1;                     /* slots consumed */
}

// Walk the current bank's node chain (+0x00 = fn, +0x08 = next) for the node
// running `fn`; a scene's own node is not the bank head.
static u32 FindSceneNode(u32 fn)
{
    u32 node = DRAW_BANK_NODE(DrawingStructArray_Count2);
    int i;

    for (i = 0; i < 16; i++)
    {
        if (node < 0x80000000 || node >= 0x81800000)
            break;
        if (VAR_ADDRESS(u32, node) == fn)
            return node;
        node = VAR_ADDRESS(u32, node + 8);
    }
    return 0;
}

// 1. The loader: point the menu-REL file-table entry at debug.rel's blob.
CGECKO(load_debug_rel_over_menus, .state = MSSB_ALWAYS,
       .notes = "Boots straight into the game's hidden developer debug menu\n"
                "instead of the main menu, with on-screen text added so it can be read.");
void load_debug_rel_over_menus(void)
{
    RelTable_PointMenuSlotAtDebugRel();
}

// 2. Capture the compiled-out menu renderer's argument (r0 is dead at entry).
CGECKO(debug_menu_capture_state, .address = 0x80048BEC, .state = MSSB_ALWAYS,
                                 .instruction = "lwz 4, 8(3)");
void debug_menu_capture_state(void)
{
    READ_GAME_REG(u32, state, 3);
    g_menuState = state;
}

// 3. The text overlay, once per frame. Gated on the live bank-node function
//    because 0x8063EFC0 can transiently read 1 before any REL loads.
CGECKO(debug_menu_text, .state = MSSB_ALWAYS);
void debug_menu_text(void)
{
    u32 node, state;
    s32 sel, count, rows, scroll, i;
    u8* items;
    int slot;

    if (g_loadedModuleId != 1 || DrawingStructArray_Count2 > 2)
        return;
    node = DRAW_BANK_NODE(DrawingStructArray_Count2);

    for (i = 0; i < TEXT_SLOTS; i++)          // release last frame's lines
        screenTextArray.blocks[TEXT_FIRST + i].state = 0;

    if (VAR_ADDRESS(u32, node) == DEBUG_SELECTOR_FN)
    {
        // top-level selector
        sel = (s32)VAR_ADDRESS(s16, node + 0x16);
        DebugLine(0, 48, 26, TXT_YELLOW, "MSSB DEBUG MENU");
        DebugList(1, 56, 72, 28,
                  "0 Stadium Viewer\0"
                  "1 Countdown (invisible)\0"
                  "2 Char Viewer (no data)\0"
                  "3 Particle Editor\0"
                  "4 Model Viewer\0"
                  "5 Sprite Viewer\0"
                  "6 Sound Test\0"
                  "7 Sprite Unit\0"
                  "8 Cutscene Player\0"
                  "9 Bat Sim Menu\0", sel);
        DebugLine(12, 48, 380, TXT_GRAY, "D-PAD MOVE  A OPEN  B+Y ALT");
        return;
    }

    if (FindSceneNode(SND_TOP_FN))
    {
        // entry 6: the Sound Test's five categories
        sel = (s32)(s8)VAR_ADDRESS(u8, SND_CURSOR);
        DebugLine(0, 48, 40, TXT_YELLOW, "SOUND TEST");
        for (i = 0; i < SND_ITEMS; i++)
        {
            const char* label = (const char*)VAR_ADDRESS(u32, SND_LABELS + i * 4);
            if ((u32)label < 0x80000000 || (u32)label >= 0x81800000)
                break;
            DebugLine(1 + i, 56, 96 + i * 28,
                      (i == sel) ? TXT_YELLOW : TXT_WHITE, label);
        }
        if (sel >= 0 && sel < SND_ITEMS)
            DebugLine(SND_ITEMS + 1, 40, 96 + sel * 28, TXT_YELLOW, ">");
        DebugLine(SND_ITEMS + 2, 48, 300, TXT_GRAY, "D-PAD MOVE  A OPEN  B BACK");
        return;
    }

    node = FindSceneNode(SND_VOICE_FN);
    if (node == 0)
        node = FindSceneNode(SND_VOICE2_FN);
    if (node)
    {
        // entry 6, items 0/1: the two Voice Tests
        int two = (VAR_ADDRESS(u32, node) == SND_VOICE2_FN);
        const char* title = (const char*)VAR_ADDRESS(u32, SND_LABELS + (two ? 4 : 0));
        const char* cap   = (const char*)VAR_ADDRESS(u32, SND_CAPTION);
        s32 v = (s32)VAR_ADDRESS(s16, two ? SND_VOICE2_NO : SND_VOICE_NO);

        if ((u32)title >= 0x80000000 && (u32)title < 0x81800000)
            DebugLine(0, 48, 40, TXT_YELLOW, title);
        if ((u32)cap >= 0x80000000 && (u32)cap < 0x81800000)
            DebugRow(1, 56, 96, TXT_WHITE, cap, &v, 0);
        DebugLine(2, 48, 300, TXT_GRAY, "L/R PICK  HOLD Y+L/R X10");
        DebugLine(3, 48, 328, TXT_GRAY, "A PLAY  B BACK");
        return;
    }

    node = FindSceneNode(SND_TRAIN_FN);
    if (node)
    {
        // entry 6, item 2: Training Se Test
        const char* title = (const char*)VAR_ADDRESS(u32, SND_LABELS + 2 * 4);
        s32 v = (s32)VAR_ADDRESS(s16, SND_TRAIN_NO);

        if ((u32)title >= 0x80000000 && (u32)title < 0x81800000)
            DebugLine(0, 48, 40, TXT_YELLOW, title);
        DebugRow(1, 56, 96, TXT_WHITE, "SE NO", &v, 0);
        DebugLine(2, 48, 300, TXT_GRAY, "L/R PICK  HOLD Y+L/R X10");
        DebugLine(3, 48, 328, TXT_GRAY, "A PLAY  B BACK");
        return;
    }

    node = FindSceneNode(SND_SE_FN);
    if (node)
    {
        // entry 6, item 3: SE Test, four slots
        const char* title = (const char*)VAR_ADDRESS(u32, SND_LABELS + 3 * 4);
        s32 row  = (s32)VAR_ADDRESS(u8, SND_SE_ROW);
        s32 mode = (s32)VAR_ADDRESS(u8, SND_SE_MODE);
        s32 v[4];
        s32 id;

        v[0] = (s32)VAR_ADDRESS(s16, SND_SE_V0);
        v[1] = (s32)VAR_ADDRESS(s16, SND_SE_V1);
        v[2] = (s32)VAR_ADDRESS(s16, SND_SE_V2);
        v[3] = (s32)VAR_ADDRESS(s16, SND_SE_V3);
        if ((u32)title >= 0x80000000 && (u32)title < 0x81800000)
            DebugLine(0, 48, 40, TXT_YELLOW, title);
        {
            const char* nm = "SE 0 SE 1 SE 2 SE 3 ";
            for (i = 0; i < 4; i++)
            {
                DebugRow(1 + i, 56, 96 + i * 28,
                         (row == i) ? TXT_YELLOW : TXT_WHITE, nm, &v[i], 0);
                nm += 5;
            }
        }
        if (row >= 0 && row <= 3)
            DebugLine(5, 40, 96 + row * 28, TXT_YELLOW, ">");
        // rows 1/2 are indices, so show what they actually resolve to
        if (row == 1)
            id = (s32)VAR_ADDRESS(u16, SND_SE_TBL1 + v[1] * 2);
        else if (row == 2)
            id = (s32)VAR_ADDRESS(u16, SND_SE_TBL2 + v[2] * 2);
        else
            id = v[row & 3];
        DebugRow(6, 56, 224, TXT_WHITE, "SOUND ID", &id, 0);
        // mode 1 (X) returns before reading any button, including B
        if (mode != 0)
            DebugLine(7, 48, 264, TXT_YELLOW, "FROZEN BY X - PRESS Y");
        DebugLine(8, 48, 300, TXT_GRAY, "UP/DN ROW   L/R VALUE");
        DebugLine(9, 48, 328, TXT_GRAY, "A PLAY   B BACK");
        return;
    }

    node = FindSceneNode(STADIUM_PICK_FN);
    if (node)
    {
        // entry 0: the stadium picker
        sel = (s32)VAR_ADDRESS(u8, STADIUM_SEL);
        DebugLine(0, 48, 40, TXT_YELLOW, "STADIUM VIEWER");
        DebugList(1, 56, 82, 26,
                  "0 Mario Stadium\0"
                  "1 Bowser Castle\0"
                  "2 Wario Palace\0"
                  "3 Yoshi Park\0"
                  "4 Peach Garden\0"
                  "5 DK Jungle\0"
                  "6 Toy Field\0", sel);
        DebugLine(9, 48, 300, TXT_GRAY, "UP/DN PICK  A LOAD");
        return;
    }

    // a scene's own menu, if its renderer ran this frame
    state = g_menuState;
    g_menuState = 0;                          // stale as soon as it stops drawing
    if (state < 0x80000000 || state >= 0x81800000)
        return;
    count  = (s32)VAR_ADDRESS(u32, state + 0x04);
    rows   = (s32)VAR_ADDRESS(u32, state + 0x08);
    scroll = (s32)VAR_ADDRESS(u32, state + 0x0C);
    sel    = (s32)VAR_ADDRESS(u32, state + 0x10);
    items  = (u8*)VAR_ADDRESS(u32, state + 0x18);
    if (count <= 0 || (u32)items < 0x80000000 || (u32)items >= 0x81800000)
        return;
    if (rows > TEXT_SLOTS - 1)
        rows = TEXT_SLOTS - 1;

    slot = 0;
    for (i = 0; i < rows; i++)
    {
        s32 idx = scroll + i;
        const char* label;

        if (idx < 0 || idx >= count)
            break;
        label = *(const char**)(items + idx * 0x2C + 4);
        if ((u32)label < 0x80000000 || (u32)label >= 0x81800000)
            break;                            // NULL label terminates the array
        {
            // +0x1C = pointer to the row's live value, +0x00 = fixed-point flag
            u32 vp = VAR_ADDRESS(u32, items + idx * 0x2C + 0x1C);
            s32 fixedPoint = (s32)VAR_ADDRESS(u32, items + idx * 0x2C + 0x00);
            if (vp < 0x80000000 || vp >= 0x81800000)
                vp = 0;
            DebugRow(slot, 56, 40 + i * 24,
                     (idx == sel) ? TXT_YELLOW : TXT_WHITE, label,
                     (const s32*)vp, fixedPoint);
        }
        if (idx == sel)
            DebugLine(TEXT_SLOTS - 1, 40, 40 + i * 24, TXT_YELLOW, ">");
        slot++;
    }
}

// 4. Silence the off-thread GX OSPanic in the two DOL texture setters.
ASM(nop_gx_thread_panic_a, "nop\n", .address = 0x800241D4, .state = MSSB_ALWAYS);
ASM(nop_gx_thread_panic_b, "nop\n", .address = 0x80024454, .state = MSSB_ALWAYS);

// 5. Entry 4 (Model Viewer): skip the SKNIt draw when the skin pointer is NULL.
CGECKO(skn_null_guard, .address = 0x800B3B24, .state = MSSB_ALWAYS,
                       .instruction = "nop");
void skn_null_guard(void)
{
    // READ_GAME_REG declares its own _sp, so read the saved frame directly for r4/r5
    READ_GAME_REG(u32, actorGeo, 3);
    u32 skin  = *(volatile u32*)(_sp + 0x8 + 4);   /* r4 */
    u32 model = *(volatile u32*)(_sp + 0x8 + 8);   /* r5 */

    if (skin < 0x80000000 || skin >= 0x81800000)
        return;                      // skin never loaded: skip this draw
    ((void (*)(u32, u32, u32))0x800BF89C)(actorGeo, skin, model);
}

// 6. Entry 1: three word patches in debug.rel's own .text, written per frame
//    while it is the loaded module (REL addresses, so never a static 04 write).
#define DEBUG_TEXT_BASE     0x8063F094      // debug.rel .text, in the menu slot
#define ENTRY1_ELEM_STORE   (DEBUG_TEXT_BASE + 0x9B6C)
#define ENTRY1_PHASE_STORE  (DEBUG_TEXT_BASE + 0x9B80)
#define ENTRY1_LOOP_CMP     (DEBUG_TEXT_BASE + 0x9B8C)

static const RioPatch ENTRY1_PATCHES[] = {
    { ENTRY1_ELEM_STORE,  0x80630000, 0x93BF0024 },
    { ENTRY1_PHASE_STORE, 0xB0030010, 0xB01D0010 },
    { ENTRY1_LOOP_CMP,    0x2C04000B, 0x2C04000A },
};

CGECKO(debug_rel_code_patches, .state = MSSB_ALWAYS);
void debug_rel_code_patches(void)
{
    if (g_loadedModuleId != 1)
        return;
    if (RioPatch_Apply(ENTRY1_PATCHES, RIO_PATCH_COUNT(ENTRY1_PATCHES), true))
    {
        // self-modifying code: one 64-byte range covers both cache lines
        DCFlushRange((void*)(ENTRY1_ELEM_STORE & ~31), 64);
        ICInvalidateRange((void*)(ENTRY1_ELEM_STORE & ~31), 64);
    }
}

// 7. removeGraphicsElementFromScene: return early for an element that owns no
//    slots (otherwise it renumbers every other element). ASM because a C2 in
//    C cannot return from the function it is injected into.
ASM(gfx_remove_empty_guard,
    "lhz 0, 0x16(3)\n"        /* r3 = the element; +0x16 = slots owned */
    "cmplwi 0, 0\n"
    "beqlr\n",                /* owns nothing -> nothing to remove */
    .address = 0x80034CEC, .state = MSSB_ALWAYS,
    .instruction = "lis 4, 0x8037");   /* the overwritten graphicsRelatedArray@ha */
