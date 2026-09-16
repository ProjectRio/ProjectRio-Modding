/*###########################################################
# Online Menu
###########################################################*/
// Author: LittleCoaks
// See docs/menu_scenes.md ("The Online button").
#include "Include/game/UnknownHomes_Game.h"
#include "Include/static/UnknownHomes_Static.h"
#include "Include/Symbols/dol.h"
#include "RioModPack/OnlineMenu.h"
#include "RioModPack/OnlineTexture.h"
#include "Include/Rio/MenuScene.h"
#include "Include/Unknown/File_0x800363d8.h"   // load_Icon
#include "Include/Unknown/File_0x800625a4.h"   // updateCharacterSelectProcessCode

// ---- game objects -----------------------------------------------------------
#define ONL_CURSOR        VAR_ADDRESS(s32, 0x80750BF0)   // lbl_2_bss_F410[0]: main-menu cursor
#define ONL_CURSOR_PREV   VAR_ADDRESS(s32, 0x80750BF4)   // [1]: where it was before the last move
#define ONL_CURSOR_SHOWN  VAR_ADDRESS(s32, 0x8074C020)   // the controller's "cursor as drawn" copy
#define ONL_SELECTED_IDX  VAR_ADDRESS(u8,  0x803530A8)   // Static_Stats_Tables.mainMenuOptionSelectedIndex
#define ONL_SCENE_ALIVE   VAR_ADDRESS(u8,  0x803530CA)   // Static_Stats_Tables+0x472A: 0 = tear the button scene down this frame
#define ONL_ITEM_NOW      MS_CURRENT_NODE                // currentDrawingItem (the running draw node)

#define ONL_CONTAINER_TAG   6      // the main menu loads entry 1740 under this tag
#define ONL_CONTAINER_NTEX  122    // ...and it has this many textures: the sanity check
#define ONL_TEX_SOURCE      49     // Exhibition Game's label: the record we copy
#define ONL_TEX_VICTIM      50     // unused on the main menu: becomes the Online label
#define ONL_PANEL_SOURCE    63     // Toy Field's preview picture: the record we copy
#define ONL_PANEL_VICTIM    51     // unused on the main menu: becomes the Online picture
#define ONL_TEX_W           156
#define ONL_TEX_H           21
#define ONL_PANEL_W         357
#define ONL_PANEL_H         302

#define ONL_ROW_PITCH_Y   MS_F32_NEG(37)   // one (squeezed) row up, in the column's space
#define ONL_ROW_FIRST_Y   137         // stock row 0's anchor after the squeeze (was 100)
#define ONL_ROW_STEP_Y    37          // stock row pitch after the squeeze (was 43)
#define ONL_ROW_COUNT     7
#define ONL_COLUMN_ELEM   56          // the column's layout element: it owns the row anchors

// Handles in the main-menu node.
#define H_COLUMN     2
#define H_BAR(i)     (3 + (i))
#define H_GREY(i)    (10 + (i))
#define H_SHINE(i)   (17 + (i))
#define H_LABEL(i)   (24 + (i))
#define H_PANEL_NEW  0x20
#define H_PANEL_OLD  0x21
#define ELEM_BAR     57
#define ELEM_GREY    59
#define ELEM_SHINE   55
#define ELEM_LABEL   53

#define R_PARENT     MS_PARENT
#define R_NEXT       MS_NEXT
#define R_CHILD      MS_CHILD
#define R_POSY       MS_POSY
#define R_FLAGS      MS_FLAGS
#define R_FRAME      MS_FRAME
#define R_ELEM       MS_ELEM
#define R_PLAY       MS_PLAY
#define R_ATTACHED   MS_ATTACHED
#define R_ICON(r)    MS_OVERRIDE(r, 1)               // part-1 texture override (the label's)
#define R_VISIBLE    MS_VISIBLE
#define FRAME(n)     MS_FRAME_OF(n)
#define BTN_REST_FRAME   FRAME(0xE)     // where the highlight bar sits once its slide has ended
#define PANEL_REST_FRAME FRAME(0xA)     // where the preview picture sits once faded in
#define ONL_ICON_TABLE   0x32                          // the preview pictures, indexed by button

// ---- our state, in claimed RAM (see ClaimedFreeMemory.h) -------------------
#define g_onlMagic       VAR_ADDRESS(u32, 0x802EC32C)
#define g_onlValid       VAR_ADDRESS(u32, 0x802EC330)   // the four copies below exist
#define g_onlItem        VAR_ADDRESS(u32, 0x802EC334)   // the main-menu draw node they belong to
#define g_onlBar         VAR_ADDRESS(u32, 0x802EC338)   // our highlight bar   (copy of handle 3)
#define g_onlGrey        VAR_ADDRESS(u32, 0x802EC33C)   // our grey bar        (copy of handle 10)
#define g_onlShine       VAR_ADDRESS(u32, 0x802EC340)   // our bar's shine     (copy of handle 17)
#define g_onlLabel       VAR_ADDRESS(u32, 0x802EC344)   // our label           (copy of handle 24)
#define g_onlArmed       VAR_ADDRESS(u32, 0x802EC348)   // A was pressed on Online: reroute the Options transition
#define g_onlAnim        VAR_ADDRESS(u32, 0x802EC34C)   // 1 while a move touching Online is animating
#define g_onlAnimNew     VAR_ADDRESS(u32, 0x802EC350)   // the arriving highlight bar record
#define g_onlAnimNewEnd  VAR_ADDRESS(u32, 0x802EC354)   // ...and the frame its slide ends on
#define g_onlAnimOld     VAR_ADDRESS(u32, 0x802EC358)   // the leaving bar, when it plays a wrap-out segment (else 0)
#define g_onlAnimOldEnd  VAR_ADDRESS(u32, 0x802EC35C)   // ...and the frame that segment ends on
#define g_onlAnimShine   VAR_ADDRESS(u32, 0x802EC360)   // the arriving bar's shine record
#define g_onlAnimCur     VAR_ADDRESS(u32, 0x802EC364)   // the cursor to publish as "drawn" when done
#define g_onlAnimLocked  VAR_ADDRESS(u32, 0x802EC368)   // input was locked for this move (not for the entry slide)
#define g_onlBg          VAR_ADDRESS(u32, 0x802EC36C)   // 1 while the backdrop records exist and the layout is recoloured
#define g_onlBgStart     VAR_ADDRESS(u32, 0x802EC370)   // saved UI element-loop start bound
#define g_onlBgEnd       VAR_ADDRESS(u32, 0x802EC374)   // saved UI element-loop end bound
#define ONL_BG_ITEM      0x802EC378                      // 24-byte scene node the backdrop records hang off
#define ONL_MAGIC        0x0A11E003

#define ONL_ENTRY        (-2)   // "prev" for the slide-in when the scene is built

static void OnlineInitState(void)
{
    if (g_onlMagic == ONL_MAGIC)
        return;
    g_onlMagic = ONL_MAGIC;
    g_onlValid = 0;
    g_onlItem  = 0;
    g_onlBar = g_onlGrey = g_onlShine = g_onlLabel = 0;
    g_onlArmed = 0;
    g_onlAnim  = 0;
    g_onlBg    = 0;
}

static void OnlineAnimTick(void);
static void MoveHighlight(u32 item, s32 prev, s32 cur);

#define ItemRecord MS_Record

// ---- the texture ------------------------------------------------------------
/* Retarget texture records 50/51 at our pixels and space the column's row
 * anchors for eight rows. Idempotent; refuses an unexpected container. */
static int PatchOnlineContainer(const u8* tex, const u8* panel)
{
    int i;
    u32 hdr = MS_TextureHeaderOfTag(ONL_CONTAINER_TAG);
    u32 layout = MS_LayoutOfTag(ONL_CONTAINER_TAG);
    u32 elem, src;

    if (hdr < 0x80000000 || layout < 0x80000000 || MS_TEX_COUNT(hdr) != ONL_CONTAINER_NTEX)
        return 0;
    src = MS_TEX(hdr, ONL_TEX_SOURCE);
    if (MS_TEX_W(src) != ONL_TEX_W || MS_TEX_H(src) != ONL_TEX_H)
        return 0;
    if (((u32)tex & 31) || ((u32)panel & 31))
        return 0;                                // the GPU needs 32-byte alignment

    MS_RetargetTexture(hdr, ONL_TEX_VICTIM, ONL_TEX_SOURCE, tex, tex + ONLINE_TEX_DATA_SIZE);

    src = MS_TEX(hdr, ONL_PANEL_SOURCE);
    if (MS_TEX_W(src) == ONL_PANEL_W && MS_TEX_H(src) == ONL_PANEL_H)
        MS_RetargetTexture(hdr, ONL_PANEL_VICTIM, ONL_PANEL_SOURCE, panel, panel + ONLINE_PANEL_DATA_SIZE);

    if (MS_ELEM_COUNT(layout) <= ONL_COLUMN_ELEM)
        return 1;
    elem = MS_LAYOUT_ELEM(layout, ONL_COLUMN_ELEM);
    if (MS_ELEM_NPARTS(elem) < ONL_ROW_COUNT + 1)
        return 1;
    for (i = 0; i < ONL_ROW_COUNT; i++)
    {
        u16 y = MS_PART_ANCHOR_Y(MS_ELEM_PART(elem, i + 1));
        if (y != 100 + 43 * i && y != ONL_ROW_FIRST_Y + ONL_ROW_STEP_Y * i)
            return 1;
    }
    for (i = 0; i < ONL_ROW_COUNT; i++)
        MS_PART_ANCHOR_Y(MS_ELEM_PART(elem, i + 1)) = ONL_ROW_FIRST_Y + ONL_ROW_STEP_Y * i;
    return 1;
}

// ---- the records ------------------------------------------------------------
/* A copy of `src` one row up, as the first child of `parent`. */
static u32 CopyRecord(u32 src, u32 parent)
{
    u32 dst = MS_CopyRecord(src, parent);
    if (dst)
        R_POSY(dst) = ONL_ROW_PITCH_Y;
    return dst;
}
#define DropRecord MS_DropRecord

static void DropAllRecords(void)
{
    // Children before parents: the shine hangs under our bar.
    DropRecord(g_onlShine);
    DropRecord(g_onlLabel);
    DropRecord(g_onlGrey);
    DropRecord(g_onlBar);
    g_onlBar = g_onlGrey = g_onlShine = g_onlLabel = 0;
    g_onlValid = 0;
    g_onlItem  = 0;
}

/* Build the Online button under the main-menu node `item`. Creation order is draw order. */
static void BuildOnlineButton(u32 item, const u8* tex, const u8* panel)
{
    u32 column = ItemRecord(item, H_COLUMN);
    u32 bar    = ItemRecord(item, H_BAR(0));
    u32 grey   = ItemRecord(item, H_GREY(0));
    u32 shine  = ItemRecord(item, H_SHINE(0));
    u32 label  = ItemRecord(item, H_LABEL(0));

    if (R_ELEM(bar) != ELEM_BAR || R_ELEM(grey) != ELEM_GREY ||
        R_ELEM(shine) != ELEM_SHINE || R_ELEM(label) != ELEM_LABEL ||
        R_PARENT(bar) != column || R_PARENT(shine) != bar)
        return;

    if (!PatchOnlineContainer(tex, panel))
        return;

    g_onlBar = CopyRecord(bar, column);
    if (g_onlBar)
    {
        R_FLAGS(g_onlBar) &= ~R_VISIBLE;        // shown only while selected
        R_FRAME(g_onlBar)  = BTN_REST_FRAME;
        R_PLAY(g_onlBar)   = 0;
    }
    g_onlGrey  = CopyRecord(grey, column);
    g_onlShine = g_onlBar ? CopyRecord(shine, g_onlBar) : 0;
    if (g_onlShine)
        R_POSY(g_onlShine) = R_POSY(shine);     // it sits in OUR bar's space: no row offset
    g_onlLabel = CopyRecord(label, column);
    if (g_onlLabel)
    {
        R_ICON(g_onlLabel) = ONL_TEX_VICTIM;
        R_FRAME(g_onlLabel) = 0;
        R_PLAY(g_onlLabel)  = 0;
    }

    if (!g_onlBar || !g_onlGrey || !g_onlShine || !g_onlLabel)
    {
        DropAllRecords();                        // pool full: no button rather than half of one
        return;
    }
    g_onlItem  = item;
    g_onlValid = 1;

    // Start on Online: the controller's entry animation reads -1 and highlights nothing.
    ONL_CURSOR_PREV = 0;
    ONL_CURSOR      = -1;
    MoveHighlight(item, ONL_ENTRY, -1);
}

// ---- lifecycle: hook the main-menu node function ----------------------------
// fn_2_747FC: first sight of a scene -> build; teardown frame -> drop our copies.
CGECKO(OnlineSceneFrame, .address = 0x806B3890, .state = MSSB_MENU,
                         .instruction = "stwu r1, -0x20(r1)",
                         .notes = "Adds an Online button to the main menu.");
void OnlineSceneFrame()
{
    u32 item = ONL_ITEM_NOW;

    OnlineInitState();
    if (ONL_SCENE_ALIVE == 0)
    {
        if (g_onlValid)
            DropAllRecords();
        g_onlAnim = 0;
        return;
    }
    if (g_onlValid && g_onlAnim)
        OnlineAnimTick();
    if (!g_onlValid && item >= 0x80000000)
        BuildOnlineButton(item, s_onlineTexture, s_onlinePanel);
}

// Safety net: free the copies if the scene went away by a route the node hook never saw.
CGECKO(OnlineRecordWatchdog, .state = MSSB_ALWAYS);
void OnlineRecordWatchdog()
{
    if (g_onlMagic != ONL_MAGIC || !g_onlValid)
        return;
    if (MS_ScreenLeft(MS_SCREEN_MAIN_MENU) ||
        R_FLAGS(ItemRecord(g_onlItem, H_LABEL(0))) == 0 ||
        R_ELEM(ItemRecord(g_onlItem, H_LABEL(0))) != ELEM_LABEL)
        DropAllRecords();
}

// ---- the cursor -------------------------------------------------------------
static void LabelIdle(u32 rec)   { R_FRAME(rec) = 0; R_PLAY(rec) = 0; }
static void LabelActive(u32 rec) { R_FRAME(rec) = 0; R_PLAY(rec) = 1; }

/* Move the highlight from `prev` to `cur` for moves that touch -1, with the
 * controller's own animation (phase 0 here, phase 1 in OnlineAnimTick). */
static void MoveHighlight(u32 item, s32 prev, s32 cur)
{
    u32 newBar   = cur  >= 0 ? ItemRecord(item, H_BAR(cur))    : g_onlBar;
    u32 newShine = cur  >= 0 ? ItemRecord(item, H_SHINE(cur))  : g_onlShine;
    u32 newLabel = cur  >= 0 ? ItemRecord(item, H_LABEL(cur))  : g_onlLabel;
    u32 oldBar   = prev >= 0 ? ItemRecord(item, H_BAR(prev))   : g_onlBar;
    u32 oldLabel = prev >= 0 ? ItemRecord(item, H_LABEL(prev)) : g_onlLabel;
    u32 start, end, mode, oldStart = 0, oldEnd = 0, oldMode = 0, oldWrap = 0;
    u32 rec;

    if (prev == ONL_ENTRY)           { start = 0;    end = 0xE;  mode = 1; }
    else if (prev == 6 && cur == -1) { start = 0;    end = 0xE;  mode = 1;   /* wrap down, off Options */
                                       oldWrap = 1; oldStart = 0x2D; oldEnd = 0x36; oldMode = 1; }
    else if (prev == -1 && cur == 6) { start = 0x36; end = 0x2D; mode = 4;   /* wrap up, off Online */
                                       oldWrap = 1; oldStart = 8;    oldEnd = 0;    oldMode = 4; }
    else if (prev < cur)             { start = 0xF;  end = 0x1D; mode = 1; } /* moving down */
    else                             { start = 0x1E; end = 0x2C; mode = 1; } /* moving up */

    if (prev != ONL_ENTRY)
    {
        if (oldWrap)
        {
            R_FLAGS(oldBar) |= R_VISIBLE;
            R_FRAME(oldBar)  = FRAME(oldStart);
            R_PLAY(oldBar)   = oldMode;
        }
        else
            R_FLAGS(oldBar) &= ~R_VISIBLE;
        LabelIdle(oldLabel);
    }

    R_FLAGS(newBar) |= R_VISIBLE;
    R_FRAME(newBar)  = FRAME(start);
    R_PLAY(newBar)   = mode;
    R_FRAME(newShine) = 0;
    R_PLAY(newShine)  = 1;
    LabelActive(newLabel);

    // The picture crossfade: incoming on handle 0x20 from frame 0, outgoing on 0x21 from 10.
    rec = ItemRecord(item, H_PANEL_NEW);
    if (cur >= 0) load_Icon((void*)item, H_PANEL_NEW, 1, ONL_ICON_TABLE, cur);
    else          R_ICON(rec) = ONL_PANEL_VICTIM;
    R_FLAGS(rec) |= R_VISIBLE;
    if (prev != ONL_ENTRY)
    {
        R_FRAME(rec) = 0; R_PLAY(rec) = 1;
        rec = ItemRecord(item, H_PANEL_OLD);
        if (prev >= 0) load_Icon((void*)item, H_PANEL_OLD, 1, ONL_ICON_TABLE, prev);
        else           R_ICON(rec) = ONL_PANEL_VICTIM;
        R_FLAGS(rec) |= R_VISIBLE; R_FRAME(rec) = PANEL_REST_FRAME; R_PLAY(rec) = 1;
        MS_makeCursorUnmovable(0);
    }

    g_onlAnimNew    = newBar;
    g_onlAnimNewEnd = end;
    g_onlAnimOld    = oldWrap ? oldBar : 0;
    g_onlAnimOldEnd = oldEnd;
    g_onlAnimShine  = newShine;
    g_onlAnimCur    = (u32)cur;
    g_onlAnimLocked = (prev != ONL_ENTRY);
    g_onlAnim       = 1;
}

/* Phase 1: stop each timeline on its end frame; when all have, publish the cursor and unlock. */
static void OnlineAnimTick(void)
{
    u32 rec;
    int done = 1;

    rec = g_onlAnimNew;
    if ((R_FRAME(rec) >> 16) == g_onlAnimNewEnd) R_PLAY(rec) = 0; else done = 0;
    rec = g_onlAnimShine;
    if ((R_FRAME(rec) >> 16) == 0xE)             R_PLAY(rec) = 0; else done = 0;
    rec = g_onlAnimOld;
    if (rec)
    {
        if ((R_FRAME(rec) >> 16) == g_onlAnimOldEnd)
        {
            R_PLAY(rec)   = 0;
            R_FLAGS(rec) &= ~R_VISIBLE;
        }
        else
            done = 0;
    }
    if (g_onlAnimLocked)
    {
        // >= not ==: a frame of the fade may slip past, and beyond 10 it fades out again.
        rec = ItemRecord(g_onlItem, H_PANEL_NEW);
        if ((R_FRAME(rec) >> 16) >= 0xA) { R_FRAME(rec) = PANEL_REST_FRAME; R_PLAY(rec) = 0; }
        else done = 0;
    }
    if (!done)
        return;
    g_onlAnim = 0;
    ONL_CURSOR_SHOWN = (s32)g_onlAnimCur;
    if (g_onlAnimLocked)
        MS_makeCursorMovable(0);
}

// D-pad branch, the `bl updateCharacterSelectProcessCode(0, 0x56)` after the wrap: both wraps become -1.
CGECKO(OnlineCursorMove, .address = 0x80641494, .state = MSSB_MENU,
                         .instruction = "nop");
void OnlineCursorMove()
{
    s32 prev = ONL_CURSOR_PREV;
    s32 cur  = ONL_CURSOR;

    OnlineInitState();
    if (g_onlValid)
    {
        if (prev == 0 && cur == 6)
            cur = -1;
        else if (prev == 6 && cur == 0)
            cur = -1;
        ONL_CURSOR = cur;
    }
    if (cur >= 0 && prev >= 0)
    {
        updateCharacterSelectProcessCode(0, 0x56);
        return;
    }
    MoveHighlight(g_onlItem, prev, cur);
}

// A branch, before `switch (cursor)`: -1 skips the unsigned-guarded jump table and
// confirms mainMenuOptionSelectedIndex, so make that Options and arm the reroute.
CGECKO(OnlineConfirm, .address = 0x80641294, .state = MSSB_MENU,
                      .instruction = "lis r4, -32651");
void OnlineConfirm()
{
    OnlineInitState();
    if (g_onlValid && ONL_CURSOR == -1)
    {
        ONL_SELECTED_IDX = 6;
        g_onlArmed = 1;
    }
}

// The "button pressed" animation indexes handle 17 + cursor; skip it for -1.
CGECKO(OnlineConfirmAnim, .address = 0x806413A4, .state = MSSB_MENU,
                          .instruction = "nop");
void OnlineConfirmAnim()
{
    OnlineInitState();
    if (!g_onlArmed)
        updateCharacterSelectProcessCode(0, 0x58);
}

// mainMenuRelated state 8, the Options transition's `bl changeScreenVariables`.
CGECKO(OnlineScreenChange, .address = 0x8064179C, .state = MSSB_MENU,
                           .instruction = "nop");
void OnlineScreenChange()
{
    OnlineInitState();
    changeScreenVariables(g_onlArmed ? ONLINE_SCREEN_CODE : 6);
    g_onlArmed = 0;
}

// ---- the placeholder screen ---------------------------------------------------
// The stock Options backdrop (tag 1 container), recoloured red in the loaded layout.
#define ONL_BG_TAG      1
#define ELEM_BG         301
#define ELEM_BG_GRAD    299
#define ELEM_TOPBAR     236
#define ELEM_TOPBAR_ART 232
#define BG_REST_FRAME   FRAME(0x28)
#define BAR_REST_FRAME  FRAME(0x14)
#define BAR_ART_FRAME   FRAME(0x7)

#define ELEM_TOPBAR_PLATE 237   /* the slanted title plate on the bar's left */
#define ELEM_TOPBAR_LINE  238   /* the thin separator under the bar           */
#define ONL_BG_COUNT 6
static const u8 s_onlBgDescs[(ONL_BG_COUNT + 1) * 0x20] = {
    MS_DESC(ELEM_BG,           0, 22, MS_NO_PARENT, ONL_BG_TAG, 0),   /* handle 0: the gradient's container */
    MS_DESC(ELEM_BG_GRAD,      0, 22, 0,            ONL_BG_TAG, 0),   /* handle 1: the gradient            */
    MS_DESC(ELEM_TOPBAR,       0,  4, MS_NO_PARENT, ONL_BG_TAG, 0),   /* handle 2: the top bar's container */
    MS_DESC(ELEM_TOPBAR_ART,   0,  4, 2,            ONL_BG_TAG, 0),   /* handle 3: the bar strip           */
    MS_DESC(ELEM_TOPBAR_PLATE, 0,  4, 2,            ONL_BG_TAG, 1),   /* handle 4: the title plate         */
    MS_DESC(ELEM_TOPBAR_LINE,  0,  4, 2,            ONL_BG_TAG, 3),   /* handle 5: the separator line      */
    MS_DESC_END
};

// [0..5] the gradient (element 299), [6] the bar strip (232), [7..8] the plate (237).
static const u32 s_onlPurple[9] = { 0x643689FF, 0x431071FF, 0x141E3CFF, 0x906FDDFF,
                                    0x7964C2FF, 0x482267FF, 0x441458FF, 0xA03ACBFF, 0xDD8CFF80 };
static const u32 s_onlRed[9]    = { 0x893636FF, 0x711010FF, 0x3C1414FF, 0xDD6F6FFF,
                                    0xC26464FF, 0x672222FF, 0x581414FF, 0xCB3A3AFF, 0xFF8C8C80 };

#define OnlBgLayout() MS_LayoutOfTag(ONL_BG_TAG)

static void OnlBgRecolour(u32 layout, const u32* from, const u32* to)
{
    MS_RecolourElement(layout, ELEM_BG_GRAD,      from,     to,     6);
    MS_RecolourElement(layout, ELEM_TOPBAR_ART,   from + 6, to + 6, 1);
    MS_RecolourElement(layout, ELEM_TOPBAR_PLATE, from + 7, to + 7, 2);
}

static void BuildBackdrop(const u8* descs, const u32* purple, const u32* red)
{
    u32 layout = OnlBgLayout();
    u32 item = ONL_BG_ITEM, lo, hi;
    int h, count;

    if (layout < 0x80000000)
        return;
    OnlBgRecolour(layout, purple, red);

    count = MS_BuildScene(item, descs);
    if (count == ONL_BG_COUNT)
    {
        R_FRAME(ItemRecord(item, 0)) = BG_REST_FRAME;  R_PLAY(ItemRecord(item, 0)) = 0;
        R_FRAME(ItemRecord(item, 1)) = 0;              R_PLAY(ItemRecord(item, 1)) = 1;
        R_FRAME(ItemRecord(item, 2)) = BAR_REST_FRAME; R_PLAY(ItemRecord(item, 2)) = 0;
        for (h = 3; h < ONL_BG_COUNT; h++)
        {
            R_FRAME(ItemRecord(item, h)) = BAR_ART_FRAME;
            R_PLAY(ItemRecord(item, h))  = 0;
        }
    }

    if (MS_SceneIndexRange(item, &lo, &hi))
        MS_NarrowDraw(lo, hi, &g_onlBgStart, &g_onlBgEnd);
    else
    {
        g_onlBgStart = MS_DRAW_START;
        g_onlBgEnd   = MS_DRAW_END;
    }
    g_onlBg = 1;
}

static void DropBackdrop(const u32* purple, const u32* red)
{
    u32 layout = OnlBgLayout();

    if (!g_onlBg)
        return;
    MS_RestoreDraw(g_onlBgStart, g_onlBgEnd);
    MS_RemoveScene(ONL_BG_ITEM);
    if (layout >= 0x80000000)
        OnlBgRecolour(layout, red, purple);
    g_onlBg = 0;
}

// Replaces the assert stub screenFuncTable[ONLINE_SCREEN_CODE] points at.
CGECKO(OnlineScene, .address = 0x806402B0, .state = MSSB_MENU,
                    .instruction = "blr");
void OnlineScene()
{
    controllerInputStruct* in = Static_Stats_Tables.controllerInputs;

    OnlineInitState();
    if (MS_MENU_PROCESS == 0)                   // first frame of this entry
    {
        MS_MENU_PROCESS = 1;
        BuildBackdrop(s_onlBgDescs, s_onlPurple, s_onlRed);
    }

    if (in[0].newInput & INPUT_BUTTON_B)
    {
        DropBackdrop(s_onlPurple, s_onlRed);
        MS_ReturnToMainMenu();
    }
}

// Safety net: drop the backdrop if the screen is left by any route but B.
static void DropOnlineBackdrop(void)
{
    DropBackdrop(s_onlPurple, s_onlRed);
}

CGECKO(OnlineBackdropWatchdog, .state = MSSB_ALWAYS);
void OnlineBackdropWatchdog()
{
    if (g_onlMagic != ONL_MAGIC || !g_onlBg)
        return;
    MS_ScreenWatchdog(ONLINE_SCREEN_CODE, DropOnlineBackdrop);
}
