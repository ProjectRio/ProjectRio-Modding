/*###########################################################
# Write Text To Screen
###########################################################*/
// Author: LittleCoaks
// Demo of the ScreenText.h helper. Text engine reference: docs/text_engine.md

#include "Include/game/UnknownHomes_Game.h"

// 3 text slots; TEXT_MAXLEN bumped for the colored item-shop demo ({tagname} spans cost a glyph each)
#define TEXT_SLOTS 3
#define TEXT_MAXLEN 96
#include "Include/Rio/ScreenText.h"

#include "Include/text/text_channel.h"

// Per frame, not hooked in the star-gauge HUD: that HUD stops drawing once the ball is in play.
CGECKO(WriteTextToScreen, .state = MSSB_GAME,
       .notes = "Developer test code. Leave this off.\n"
                "Draws sample text on the in-game screen while a play is live.");
void WriteTextToScreen()
{
    // free last frame's texts even on frames that draw nothing
    ScreenTextTick();

    if (g_GameLogic.sceneID != SCENE_ID_LIVE_BALL)
        return;

    // // if player 1 is holding Z
    // if ((InputBuffer[0].button & (INPUT_TRIGGER_Z)) != (INPUT_TRIGGER_Z))
    //   return;

    // %08f zero-pads so the decimal points line up (the font has no tab)
    WriteTextEx(0, 0, TEXT_YELLOW, TEXT_SMALL, TEXT_LEFT,
                "Contact frame: %d\n"
                "Ball X: %08f\n"
                "Ball Y: %08f\n"
                "Ball Z: %08f",
                g_Batter.Stored_Frame_SwingContact_SinceMiss,
                g_Ball.AtBat_Contact_BallPos.x, g_Ball.AtBat_Contact_BallPos.y, g_Ball.AtBat_Contact_BallPos.z);

    // menu-style paragraph, wrapped at 20 glyphs per line
    WriteMenuText(20, 120, 20, "Wrap demo: this text flows onto lines");

    // the Challenge Mode item shop's "mysterious power" popup, with {tagname} inline colours
    WriteTextEx(20, 220, 0x323232FF, TEXT_SMALL, TEXT_LEFT,
                "A mysterious power makes it\n"
                "{red}easier{reset} for everyone {red}to get hits{reset}.\n"
                "{blue}(Only good next game.){reset}");
}
