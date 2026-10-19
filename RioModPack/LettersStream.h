/*###########################################################
# LettersStream.h -- the unused ZZZZ.dat song, streamed through MusyX
###########################################################*/
// Author: LittleCoaks
//
// The orphan "Letters" song out of ZZZZ.dat, on Include/Rio/MusyxStream.h.
// Include from ONE hook only; LettersStream_Start/Pump/Stop/Active are the
// engine's calls. See docs/musyx_stream.md.
#ifndef LETTERSSTREAM_H
#define LETTERSSTREAM_H

#include "Include/types.h"

/* The song's own .dsp header coefficients (blob +0x44). */
static const s16 kLettersCoef[16] = {
    -184,   95, 1855, -724,  783,  609, 2500, -654,
     906, -480, 2398, -799, 1460,  366, 2801, -818
};

#define LETTERS_SONG_BLOB       0x08F2E808u                 /* within ZZZZ.dat            */

#define MUSYXSTREAM_FILE        "ZZZZ.dat"
#define MUSYXSTREAM_DATA_OFFSET (LETTERS_SONG_BLOB + 0x88u) /* past AdGCForm + .dsp hdr   */
#define MUSYXSTREAM_DATA_BYTES  0x507340u                   /* whole frames, 32-aligned   */
#define MUSYXSTREAM_FRQ         32000u
#define MUSYXSTREAM_COEF        kLettersCoef
#define MUSYXSTREAM_STID        0x4C455454u                 /* 'LETT' */
#include "Include/Rio/MusyxStream.h"

#define LettersStream_Active MusyxStream_Active
#define LettersStream_Start  MusyxStream_Start
#define LettersStream_Pump   MusyxStream_Pump
#define LettersStream_Stop   MusyxStream_Stop

#endif /* LETTERSSTREAM_H */
