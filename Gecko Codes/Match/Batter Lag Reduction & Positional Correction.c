/*###########################################################
# Batter Lag Reduction & Positional Correction
###########################################################*/
// Author: LittleCoaks

#define BATTING_PREDICTION_NOTES                         \
    "For laggy displays or netplay: plays as if one\n"   \
    "frame of input delay were removed.\n"               \
    "Contact is judged as if the swing came a frame\n"   \
    "earlier, and the batter's box movement lands\n"     \
    "where a frame-earlier input would have put it."

#include "Gecko Codes/Match/BattingPrediction.h"
