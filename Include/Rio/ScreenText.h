/*###########################################################
# ScreenText.h -- printf-style text on the in-game HUD
###########################################################*/
// Author: LittleCoaks
//
// Draws through the game's own text engine. From a per-frame code:
//     ScreenTextTick();                                   // once, top of the hook
//     WriteText(320, 36, "Inning %d", inning);            // white, large, centred
//     WriteTextEx(16, 400, TEXT_YELLOW, TEXT_SMALL, TEXT_LEFT, "P%d", port + 1);
// Text lives one frame; {red}..{reset} markup recolours mid-string.
// Engine internals, format codes, limits and buffer placement: docs/text_engine.md.

#ifndef SCREENTEXT_H
#define SCREENTEXT_H

#include "Include/game/UnknownHomes_Game.h"
#include "Include/static/UnknownHomes_Static.h"
#include "Include/text/text_channel.h"

#define TEXT_LEFT   0
#define TEXT_CENTER 1
#define TEXT_RIGHT  2

#define TEXT_LARGE  0   /* 22px */
#define TEXT_SMALL  1   /* 18px */

/* 0xRRGGBBAA */
#define TEXT_WHITE  0xFFFFFFFF
#define TEXT_BLACK  0x000000FF
#define TEXT_RED    0xFF2020FF
#define TEXT_GREEN  0x20FF20FF
#define TEXT_BLUE   0x4060FFFF
#define TEXT_YELLOW 0xFFFF20FF
#define TEXT_ORANGE 0xFF9020FF
#define TEXT_GRAY   0x808080FF

/* The game's own inline recolour palette (opcodes 6-14), for picking a matching base colour. */
#define TEXT_PALETTE_PINK   0xFF15B1FF   /* opcode 6 */
#define TEXT_PALETTE_GOLD   0xB89000FF   /* opcode 7 */
#define TEXT_PALETTE_RED    0xFF0000FF   /* opcode 8 */
#define TEXT_PALETTE_GREEN  0x009900FF   /* opcode 9 */
#define TEXT_PALETTE_BLUE   0x0000FFFF   /* opcode 10 */
#define TEXT_PALETTE_MAROON 0x800000FF   /* opcode 11 */
#define TEXT_PALETTE_TEAL   0x339966FF   /* opcode 12 */
#define TEXT_PALETTE_PURPLE 0x333399FF   /* opcode 13 */
#define TEXT_PALETTE_BLACK  0x000000FF   /* opcode 14 */

#ifndef TEXT_SLOTS
#define TEXT_SLOTS  8
#endif
#ifndef TEXT_MAXLEN
#define TEXT_MAXLEN 47
#endif
#ifndef TEXT_FIRST_BLOCK
#define TEXT_FIRST_BLOCK (30 - TEXT_SLOTS)
#endif

/* Frame stamp for freeing last frame's slots. The default never advances in
 * menus; a menu code must #define its own counter before the include. */
#ifndef ScreenText_FrameNow
#define ScreenText_FrameNow g_d_GameSettings.FrameCountWhileNotAtMainMenu
#endif

/* The optimize pragma resets -f flags, so CGecko's correctness flags are restated. */
#pragma GCC push_options
#pragma GCC optimize ("Os,no-jump-tables,no-optimize-sibling-calls,no-tree-loop-distribute-patterns")

typedef struct
{
    u32 lastFrame;
    u32 nextSlot;
    u16 bufs[TEXT_SLOTS][TEXT_MAXLEN + 1];
} ScreenTextState;

#ifdef TEXT_BUFFER_ADDR
#define s_screenText VAR_ADDRESS(ScreenTextState, TEXT_BUFFER_ADDR)
#else
static ScreenTextState s_screenText = { 0xFFFFFFFF, 0, {{0}} };   /* nonzero init keeps it in .data */
#endif

/* ASCII -> MSSB glyph code. */
static u16 ScreenText_Encode(char c)
{
    if (c == ' ')             return 0x4002;
    if (c == '\n')            return 0x4001;
    if (c >= 'a' && c <= 'z') return c - 'a' + 62;
    if (c == '{')             return 88;
    if (c == '}')             return 89;
    if (c == '~')             return 90;
    if (c == ']')             return 59;
    if (c == '^')             return 60;
    if (c >= '!' && c <= '[') return c - '!';
    return 30;                /* '?' */
}

static int ScreenText_Number(u16* buf, u32 v, u32 base, bool negative)
{
    u16 rev[10];              /* u32 max: 10 digits */
    int cnt = 0;
    do
    {
        u32 d = v % base;
        rev[cnt++] = (d < 10) ? 15 + d : 32 + (d - 10);
        v /= base;
    } while (v != 0 && cnt < 10);

    int len = 0;
    if (negative)
        buf[len++] = 12;      /* '-' */
    while (cnt > 0)
        buf[len++] = rev[--cnt];
    return len;
}

/* Two decimals, trailing zeros kept. Varargs promote float to double. */
static int ScreenText_Float(u16* buf, double v)
{
    bool neg = v < 0.0;
    if (neg)
        v = -v;

    u32 ipart = (u32)v;                             /* whole part */
    u32 fdig  = (u32)((v - (double)ipart) * 100.0 + 0.5);  /* 2 dp, rounded, 0..100 */
    if (fdig >= 100)                                /* rounding carried into ones */
    {
        fdig -= 100;
        ipart += 1;
    }

    int len = ScreenText_Number(buf, ipart, 10, neg);  /* sign + integer part */
    buf[len++] = 13;                                /* '.' */
    buf[len++] = 15 + fdig / 10;                    /* tens digit */
    buf[len++] = 15 + fdig % 10;                    /* ones digit */
    return len;
}

/* p points just past a '{'. Returns the tag length and its opcode, or 0. */
static int ScreenText_MatchTag(const char* p, int* outOpcode)
{
    static const struct { const char* name; int opcode; } tags[] = {
        { "reset",  5 }, { "pink",  6 }, { "gold",   7 }, { "red",    8 },
        { "green",  9 }, { "blue", 10 }, { "maroon", 11 }, { "teal",  12 },
        { "purple", 13 }, { "black", 14 },
    };
    for (int t = 0; t < 10; t++)
    {
        const char* name = tags[t].name;
        int len = 0;
        while (name[len] != 0)
            len++;
        bool match = true;
        for (int k = 0; k < len; k++)
        {
            if (p[k] != name[k])
            {
                match = false;
                break;
            }
        }
        if (match && p[len] == '}')
        {
            *outOpcode = tags[t].opcode;
            return len;
        }
    }
    return 0;
}

/* On the first call of each frame: free our blocks, restart the slot count. */
static void ScreenTextTick(void)
{
    u32 frame = ScreenText_FrameNow;
    if (s_screenText.lastFrame == frame)
        return;
    s_screenText.lastFrame = frame;
    s_screenText.nextSlot  = 0;
    for (int i = 0; i < TEXT_SLOTS; i++)
        screenTextArray.blocks[TEXT_FIRST_BLOCK + i].state = 0;
}

static void WriteTextV(int x, int y, u32 color, int style, int justify,
                       int maxChars, const char* fmt, __builtin_va_list args)
{
    ScreenTextTick();
    if (s_screenText.nextSlot >= TEXT_SLOTS)
        return;
    int slot = (int)s_screenText.nextSlot++;

    u16* out = s_screenText.bufs[slot];
    int  n   = 0;
    for (const char* p = fmt; *p != 0 && n < TEXT_MAXLEN; p++)
    {
        if (*p == '{')
        {
            int opcode;
            int taglen = ScreenText_MatchTag(p + 1, &opcode);
            if (taglen > 0)
            {
                out[n++] = (u16)(0x4000 | opcode);
                p += taglen + 1;   /* now at '}'; the for-loop's p++ clears it */
                continue;
            }
        }

        if (*p != '%')
        {
            out[n++] = ScreenText_Encode(*p);
            continue;
        }

        /* %[0][width]<conv> */
        p++;
        bool zeroPad = (*p == '0');
        if (zeroPad)
            p++;
        int width = 0;
        while (*p >= '0' && *p <= '9')
            width = width * 10 + (*p++ - '0');

        u16 num[24];          /* formatted numeric field, before padding */
        int len = -1;         /* >= 0 once a numeric conversion filled num[] */
        if (*p == 'd')
        {
            int v = __builtin_va_arg(args, int);
            len = ScreenText_Number(num, v < 0 ? -(u32)v : (u32)v, 10, v < 0);
        }
        else if (*p == 'u')
            len = ScreenText_Number(num, __builtin_va_arg(args, u32), 10, false);
        else if (*p == 'x')
            len = ScreenText_Number(num, __builtin_va_arg(args, u32), 16, false);
#ifndef SCREENTEXT_NO_FLOAT
        else if (*p == 'f')
            len = ScreenText_Float(num, __builtin_va_arg(args, double));
#endif

        else if (*p == 's')
        {
            for (const char* s = __builtin_va_arg(args, const char*);
                 *s != 0 && n < TEXT_MAXLEN; s++)
                out[n++] = ScreenText_Encode(*s);
        }
        else if (*p == '%')
            out[n++] = ScreenText_Encode('%');
        else if (*p == 0)
            break;
        /* unknown %-code: dropped */

        if (len >= 0)         /* emit a numeric field, left-padded to width */
        {
            int padCount = width - len;
            int start = 0;
            if (zeroPad && len > 0 && num[0] == 12 /* '-' */)
            {
                if (n < TEXT_MAXLEN) out[n++] = 12;   /* sign before the zero padding */
                start = 1;
            }
            u16 padGlyph = zeroPad ? 15 /* '0' */ : 0x4002 /* space */;
            for (int i = 0; i < padCount && n < TEXT_MAXLEN; i++)
                out[n++] = padGlyph;
            for (int i = start; i < len && n < TEXT_MAXLEN; i++)
                out[n++] = num[i];
        }
    }

    /* word-wrap at spaces so no line exceeds maxChars glyphs */
    if (maxChars > 0)
    {
        int lineStart = 0, lastSpace = -1;
        for (int i = 0; i < n; i++)
        {
            u16 g = out[i];
            if (g == 0x4001)                    /* existing newline resets the line */
            {
                lineStart = i + 1;
                lastSpace = -1;
            }
            else if (g == 0x4002)               /* space: a break candidate */
            {
                if (i - lineStart >= maxChars)  /* line already full: break here */
                {
                    out[i] = 0x4001;
                    lineStart = i + 1;
                    lastSpace = -1;
                }
                else
                    lastSpace = i;
            }
            else if (i - lineStart >= maxChars && lastSpace > lineStart)
            {
                out[lastSpace] = 0x4001;        /* overflowed: break at last space */
                lineStart = lastSpace + 1;
                lastSpace = -1;
            }
        }
    }

    out[n] = 0x4000;          /* end of string */

    ScreenText* t = &screenTextArray.blocks[TEXT_FIRST_BLOCK + slot];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
    /* blocks are 4-aligned in RAM despite the packed struct decl */
    u32* raw = (u32*)t;
#pragma GCC diagnostic pop
    for (int i = 0; i < 14; i++)                             /* 14 words = 56 bytes */
        raw[i] = 0;
    t->bankText             = (u16*)out;
    t->color                       = (s32)color;             /* RGBA */
    t->x                        = (u16)x;
    t->y                        = (u16)y;
    t->maxLettersToDraw        = -1;                     /* -1 = show all */
    t->drawGroup                = 5;                      /* 1-8 drawn every frame */
    t->style                  = (u8)style;
    t->lineSpacing                = 2;
    t->justify = (u8)justify;
    t->state                = 2;                      /* active -- set last */
}

/* Full control over color (RGBA), font size, and justification. */
static void WriteTextEx(int x, int y, u32 color, int style, int justify,
                        const char* fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    WriteTextV(x, y, color, style, justify, 0, fmt, args);
    __builtin_va_end(args);
}

/* White large text centered on x. */
static void WriteText(int x, int y, const char* fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    WriteTextV(x, y, TEXT_WHITE, TEXT_LARGE, TEXT_CENTER, 0, fmt, args);
    __builtin_va_end(args);
}

/* A paragraph word-wrapped at maxChars glyphs per line. */
static void WriteTextWrapped(int x, int y, u32 color, int style, int justify,
                             int maxChars, const char* fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    WriteTextV(x, y, color, style, justify, maxChars, fmt, args);
    __builtin_va_end(args);
}

/* The Records/menu look: small, white, left-justified, wrapped at maxChars. */
static void WriteMenuText(int x, int y, int maxChars, const char* fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    WriteTextV(x, y, TEXT_WHITE, TEXT_SMALL, TEXT_LEFT, maxChars, fmt, args);
    __builtin_va_end(args);
}

#pragma GCC pop_options

#endif /* SCREENTEXT_H */
