/*###########################################################
# ScreenList.h -- a scrollable list of items with a cursor
###########################################################*/
// Author: LittleCoaks
//
// A menu-list widget on top of ScreenText.h: ScreenList_Init(list, count,
// visibleRows) once, then ScreenList_MoveUp/MoveDown on your own edge-detected
// input and ScreenList_Draw(list, x, y, rowSpacing, style, labels) each frame;
// list->selected is the chosen index. Size TEXT_SLOTS to at least
// visibleRows + 1. See docs/text_engine.md.

#ifndef SCREENLIST_H
#define SCREENLIST_H

#include "Include/Rio/ScreenText.h"
#include "Include/text/text_channel.h"

#ifndef LIST_CURSOR_WIDTH
#define LIST_CURSOR_WIDTH 20
#endif

typedef struct
{
    int count;        /* total number of items in the list */
    int selected;      /* current selection, 0..count-1 */
    int scrollTop;     /* index of the first row currently drawn */
    int visibleRows;   /* how many rows are shown at once (scroll window) */
} ScreenList;

static void ScreenList_Init(ScreenList* list, int count, int visibleRows)
{
    list->count       = count;
    list->selected    = 0;
    list->scrollTop    = 0;
    list->visibleRows = visibleRows;
}

static void ScreenList_ScrollToSelection(ScreenList* list)
{
    if (list->selected < list->scrollTop)
        list->scrollTop = list->selected;
    else if (list->selected >= list->scrollTop + list->visibleRows)
        list->scrollTop = list->selected - list->visibleRows + 1;
}

static void ScreenList_MoveDown(ScreenList* list)
{
    if (list->count <= 0)
        return;
    list->selected++;
    if (list->selected >= list->count)
        list->selected = 0;              /* wrap to the top */
    ScreenList_ScrollToSelection(list);
}

static void ScreenList_MoveUp(ScreenList* list)
{
    if (list->count <= 0)
        return;
    list->selected--;
    if (list->selected < 0)
        list->selected = list->count - 1; /* wrap to the bottom */
    ScreenList_ScrollToSelection(list);
}

/* One row of a list the caller lays out itself: the cursor (when selected)
 * at x and the label at x + LIST_CURSOR_WIDTH. Returns whether row `i` is
 * the selected one, for the caller's own columns. */
static bool ScreenList_DrawRow(ScreenList* list, int i, int x, int y, int style,
                               const char* label)
{
    bool isSelected = (i == list->selected);

    if (isSelected)
        WriteTextEx(x, y, TEXT_YELLOW, style, TEXT_LEFT, ">");
    WriteTextEx(x + LIST_CURSOR_WIDTH, y, isSelected ? TEXT_YELLOW : TEXT_WHITE,
                style, TEXT_LEFT, "%s", label);
    return isSelected;
}

static void ScreenList_Draw(ScreenList* list, int x, int y, int rowSpacing,
                            int style, const char* const* labels)
{
    int first = list->scrollTop;
    int last  = first + list->visibleRows;
    if (last > list->count)
        last = list->count;

    for (int i = first; i < last; i++)
    {
        int row  = i - first;
        int rowY = y + row * rowSpacing;
        bool isSelected = (i == list->selected);

        if (isSelected)
            WriteTextEx(x, rowY, TEXT_YELLOW, style, TEXT_LEFT, "{gold}>{reset}");
        WriteTextEx(x + LIST_CURSOR_WIDTH, rowY, isSelected ? TEXT_YELLOW : TEXT_WHITE,
                    style, TEXT_LEFT, "%s", labels[i]);
    }
}

#endif /* SCREENLIST_H */
