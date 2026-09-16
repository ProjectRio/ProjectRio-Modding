/*###########################################################
# Options Menu
###########################################################*/
// Author: LittleCoaks
// See docs/menu_scenes.md ("The Options menu") and docs/text_engine.md.
#include "Include/game/UnknownHomes_Game.h"

// Text slot budget: 3 + OPT_VISIBLE_ROWS*2 + NOTES_MAX_LINES = 22 (see docs/text_engine.md).
#define SCREENTEXT_NO_FLOAT
#define TEXT_SLOTS 22
#define TEXT_MAXLEN 27
#define TEXT_BUFFER_ADDR 0x802EB050   // 1240 bytes, see ClaimedFreeMemory.h

// Our own frame clock: the default stamp never advances in menus, and it must
// be claimed RAM, not a payload static (r31 is NULL inside helpers).
#define g_frame VAR_ADDRESS(u32, 0x802EB528)   // right after the text buffer
#define ScreenText_FrameNow g_frame

#include "Include/Rio/ScreenList.h"
#include "Include/Rio/MenuScene.h"

#include "RioModPack/ModOptions.h"
#include "RioModPack/MusicConfig.h"
#include "Include/static/UnknownHomes_Static.h"
#include "Include/text/text_channel.h"
#include "RioModPack/OnlineMenu.h"       // ONLINE_SCREEN_CODE, for the watchdog

// Mutable state lives at claimed-RAM addresses, never in payload statics.
#define g_magic VAR_ADDRESS(u32, 0x802EC308)
#define s_list  ((ScreenList*)0x802EC30C)          // 16 bytes

// ---- the music sub-screen -------------------------------------------------
#define PAGE_OPTIONS 0
#define PAGE_MUSIC   1
#define g_page   VAR_ADDRESS(u32, 0x802EB52C)
#define s_music  ((ScreenList*)0x802EB530)          // 16 bytes

// ---- BACKGROUND -----------------------------------------------------------
#define NOTHING_SAVED    0xFFFFFFFF

#define g_savedDrawEnd   VAR_ADDRESS(u32, 0x802EB000)
#define g_bgMagic        VAR_ADDRESS(u32, 0x802EB004)
#define g_savedDrawStart VAR_ADDRESS(u32, 0x802EB008)
#define BG_MAGIC        0x0B6D0FF

/* Blank everything the menu framework draws, keeping the text pass. */
static void BlankBackground(void)
{
    if (g_savedDrawEnd == NOTHING_SAVED && MS_DRAW_END != 0)
        MS_BlankDraw(&g_savedDrawStart, &g_savedDrawEnd);   /* only ever saves the real bound */
}

/* Give the framework its screen back. Safe to call when nothing is saved. */
static void RestoreBackground(void)
{
    MS_RestoreDraw(g_savedDrawStart, g_savedDrawEnd);   /* NOTHING_SAVED is out of range: no write */
    g_savedDrawEnd = NOTHING_SAVED;
}

// ---- layout (the full 4:3 frame) ------------------------------------------
#define OPT_VISIBLE_ROWS 5
#define OPT_CURSOR_X    32                  // labels at + LIST_CURSOR_WIDTH
#define OPT_VALUE_X    250
#define OPT_TOP_Y      108
#define OPT_ROW_H       34

#define OPT_TITLE_X     32
#define OPT_TITLE_Y     52
#define OPT_FOOTER_Y   396

// The notes panel. Avoid '_' in a note: the font has no underscore glyph.
#define NOTES_X        310
#define NOTES_TOP_Y    108
#define NOTES_LINE_H    26
#define NOTES_MAX_LINES  9
#define NOTES_MAX_CHARS 24

// ---- the rows ------------------------------------------------------------
typedef struct
{
    const char* label;      /* <= 18 glyphs, see TEXT_MAXLEN above */
    u32         addr;       /* the word this row toggles, or just its notes key */
    u32         onValue;    /* what ON writes; OFF always writes 0 */
    u32         page;       /* PAGE_OPTIONS = a toggle; else the screen A opens */
} ModOptionRow;

// Only the mods RioModPack actually ships get a row.
static const ModOptionRow s_options[] =
{
    { "Widescreen",      MODOPT_ADDR(MODOPT_WIDESCREEN), 1, PAGE_OPTIONS },
    { "Duplicate Chars", MODOPT_ADDR(MODOPT_DUPLICATES), 1, PAGE_OPTIONS },
    { "Custom Music",    MODOPT_ADDR(MODOPT_MUSIC),      1, PAGE_MUSIC   },
    { "Gecko Codes",     MODOPT_ADDR(MODOPT_GECKO),      1, PAGE_OPTIONS },
    { "Night Stadium",   MODOPT_ADDR(MODOPT_NIGHT_MARIO), 1, PAGE_OPTIONS },
    { "Swing Skip",      MODOPT_ADDR(MODOPT_SWING_SKIP), 1, PAGE_OPTIONS },
};
#define OPT_COUNT ((int)(sizeof(s_options) / sizeof(s_options[0])))

// ---- the music screen -----------------------------------------------------
#define MUSIC_VISIBLE_ROWS 9
#define MUSIC_TRACK_X    250
#define MUSIC_TOP_Y      108
#define MUSIC_ROW_H       30
#define MUSIC_FOOTER_Y   396

// ---- the notes panel ------------------------------------------------------
/* The selected mod's .notes, word-wrapped one text block per line. */
static void DrawNotes(u32 optionAddr)
{
    const char* note = CGecko_NotesForOption(optionAddr);
    int line;

    if (note == 0)
        return;                      // mod declared no .notes, or gecko build

    for (line = 0; line < NOTES_MAX_LINES; line++)
    {
        char buf[NOTES_MAX_CHARS + 1];
        int  take, lastSpace, i;

        while (*note == ' ')                 // the space we broke at
            note++;
        if (*note == '\n')                   // a break the author asked for
            note++;
        while (*note == ' ')
            note++;
        if (*note == 0)
            break;

        take = 0;
        lastSpace = -1;
        while (note[take] != 0 && note[take] != '\n' && take < NOTES_MAX_CHARS)
        {
            if (note[take] == ' ')
                lastSpace = take;
            take++;
        }
        if (take == NOTES_MAX_CHARS && note[take] != 0 && note[take] != '\n' &&
            note[take] != ' ' && lastSpace > 0)
            take = lastSpace;                // we stopped mid-word: back up

        for (i = 0; i < take; i++)
            buf[i] = note[i];
        buf[take] = 0;
        note += take;

        if (line == NOTES_MAX_LINES - 1)     // out of lines with text left: end in "..."
        {
            const char* rest = note;

            while (*rest == ' ' || *rest == '\n')
                rest++;
            if (*rest != 0)
            {
                int at = (take <= NOTES_MAX_CHARS - 3) ? take : NOTES_MAX_CHARS - 3;

                buf[at]     = '.';
                buf[at + 1] = '.';
                buf[at + 2] = '.';
                buf[at + 3] = 0;
            }
        }

        WriteTextEx(NOTES_X, NOTES_TOP_Y + line * NOTES_LINE_H,
                    TEXT_WHITE, TEXT_SMALL, TEXT_LEFT, "%s", buf);
    }
}

// Replaces screenFuncTable[6] (Options): called once per frame while screenCode == 6.
CGECKO(OptionsMenu, .address = 0x80658D98, .state = MSSB_MENU,
                    .instruction = "blr",
                    .notes = "Adds a Mods menu to the Options screen where each "
                             "mod can be turned on or off.");
void OptionsMenu()
{
    controllerInputStruct* in;
    u16 pressed;
    int i, first, last;

    if (g_magic != 0x0D71104)           // claimed RAM holds whatever was there
    {                                    // at power-on -- zero the flags ONCE
        g_magic = 0x0D71104;             // per session, not on every entry
        g_savedDrawEnd = NOTHING_SAVED;
        ModOptions_Reset();
    }

    if (MS_MENU_PROCESS == 0)           // first frame of this entry
    {
        MS_MENU_PROCESS = 1;
        BlankBackground();
        ScreenList_Init(s_list, OPT_COUNT, OPT_VISIBLE_ROWS);
        g_page = PAGE_OPTIONS;          // always open on the options list
    }

    g_frame++;                          // before the B exit, so text vanishes on the way out
    ScreenTextTick();

    in = Static_Stats_Tables.controllerInputs;
    pressed = in[0].newInput;           // already edge-detected by the menu's input gather

    // ---- the music screen -------------------------------------------------
    if (g_page == PAGE_MUSIC)
    {
        char scratch[MUSIC_PATHBUF_SIZE];   // MusicTrackStep builds paths here
        int  slot;

        if (pressed & INPUT_BUTTON_B)
        {
            g_page = PAGE_OPTIONS;
            return;                      // draw nothing on the frame we leave
        }
        if (pressed & INPUT_BUTTON_DOWN)
            ScreenList_MoveDown(s_music);
        if (pressed & INPUT_BUTTON_UP)
            ScreenList_MoveUp(s_music);

        slot = s_music->selected;
        if (pressed & INPUT_BUTTON_RIGHT)
            MusicSlot(slot) = MusicTrackStep(slot, MusicSlotTrack(slot), +1, scratch);
        if (pressed & INPUT_BUTTON_LEFT)
            MusicSlot(slot) = MusicTrackStep(slot, MusicSlotTrack(slot), -1, scratch);

        WriteTextEx(OPT_TITLE_X, OPT_TITLE_Y, TEXT_WHITE, TEXT_LARGE, TEXT_LEFT,
                    "MUSIC");

        first = s_music->scrollTop;
        last  = first + s_music->visibleRows;
        if (last > MUSIC_SLOT_COUNT)
            last = MUSIC_SLOT_COUNT;

        for (i = first; i < last; i++)
        {
            int  y   = MUSIC_TOP_Y + (i - first) * MUSIC_ROW_H;
            u32  trk = MusicSlotTrack(i);

            // Red = configured, but that file is not on this disc (only an ini code can do that).
            u32 live = (trk == MUSIC_DEFAULT) || MusicTrackAvailable(i, trk, scratch);

            ScreenList_DrawRow(s_music, i, OPT_CURSOR_X, y, TEXT_SMALL, s_musicSlotLabel[i]);
            WriteTextEx(MUSIC_TRACK_X, y,
                        !live ? TEXT_RED
                              : (trk == MUSIC_DEFAULT || trk == MUSIC_OFF) ? TEXT_GRAY
                              : TEXT_GREEN,
                        TEXT_SMALL, TEXT_LEFT, "%s", s_musicTrackLabel[trk]);
        }

        WriteTextEx(OPT_TITLE_X, MUSIC_FOOTER_Y, TEXT_GRAY, TEXT_SMALL, TEXT_LEFT,
                    "L/R: TRACK  B: BACK");
        return;
    }

    if (pressed & INPUT_BUTTON_B)
    {
        RestoreBackground();            // give the framework its screen back
        changeScreenVariables(5);       // ...then hand over to the main menu
        return;                          // draw nothing on the exit frame
    }
    if (pressed & INPUT_BUTTON_DOWN)
        ScreenList_MoveDown(s_list);
    if (pressed & INPUT_BUTTON_UP)
        ScreenList_MoveUp(s_list);
    if (pressed & INPUT_BUTTON_A)
    {
        const ModOptionRow* row = &s_options[s_list->selected];

        if (row->page != PAGE_OPTIONS)
        {
            g_page = row->page;
            ScreenList_Init(s_music, MUSIC_SLOT_COUNT, MUSIC_VISIBLE_ROWS);
            return;                      // the new screen draws from next frame
        }
        {
            u32* flag = (u32*)row->addr;
            *flag = *flag ? 0 : row->onValue;
        }
    }

    // ---- draw ------------------------------------------------------------
    WriteTextEx(OPT_TITLE_X, OPT_TITLE_Y, TEXT_WHITE, TEXT_LARGE, TEXT_LEFT,
                "MOD OPTIONS");

    first = s_list->scrollTop;
    last  = first + s_list->visibleRows;
    if (last > OPT_COUNT)
        last = OPT_COUNT;

    for (i = first; i < last; i++)
    {
        int  y   = OPT_TOP_Y + (i - first) * OPT_ROW_H;
        bool on  = (*(u32*)s_options[i].addr != 0);

        ScreenList_DrawRow(s_list, i, OPT_CURSOR_X, y, TEXT_SMALL, s_options[i].label);
        if (s_options[i].page != PAGE_OPTIONS)
            WriteTextEx(OPT_VALUE_X, y, TEXT_GRAY, TEXT_SMALL, TEXT_LEFT, "SET");
        else
            WriteTextEx(OPT_VALUE_X, y, on ? TEXT_GREEN : TEXT_GRAY,
                        TEXT_SMALL, TEXT_LEFT, on ? "ON" : "OFF");
    }

    DrawNotes(s_options[s_list->selected].addr);

    WriteTextEx(OPT_TITLE_X, OPT_FOOTER_Y, TEXT_GRAY, TEXT_SMALL, TEXT_LEFT,
                "A: SELECT  B: BACK");
}

// Safety net: un-blank the UI if the Options screen is left by any route but B.
// MSSB_ALWAYS because the blanked variable is main.dol state shared with the match.
static void OptionsScreenLeft(void)
{
    RestoreBackground();
    g_frame++;                           // ...and release our text blocks too
    ScreenTextTick();
}

CGECKO(OptionsMenuRestore, .state = MSSB_ALWAYS);
void OptionsMenuRestore()
{
    if (g_bgMagic != BG_MAGIC)          // one-shot, before any restore can run
    {
        g_bgMagic      = BG_MAGIC;
        g_savedDrawEnd = NOTHING_SAVED;
        return;
    }
    if (g_savedDrawEnd == NOTHING_SAVED) // nothing blanked -- the common case
        return;

    if (!MS_MenuCtrlValid())
        return;                          // no menu control struct yet (boot)
    if (MS_SCREEN_CODE != ONLINE_SCREEN_CODE)   // the Online screen blanks the same way
        MS_ScreenWatchdog(MS_SCREEN_OPTIONS, OptionsScreenLeft);
}
