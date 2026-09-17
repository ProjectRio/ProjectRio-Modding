/*###########################################################
# Widescreen
###########################################################*/
// Author: LittleCoaks
// How the EFB scaling, projection hooks, HUD pinning and never-cull patches work: docs/widescreen.md

#include "Include/game/UnknownHomes_Game.h"

#include "Include/static/UnknownHomes_Static.h"

// (4/3) / (16/9) = 0.75. For other displays use (4.0/3.0) / (your aspect).
#define WIDESCREEN_X_SCALE 0.75f

/*-----------------------------------------------------------
 Section 1: never-cull patches (no .address -> once per frame)
-----------------------------------------------------------*/

// .notes goes on the first of this file's four hooks; they all share one option.
CGECKO(WidescreenNeverCull,
       .notes = "Render the whole game in 16:9. Set Aspect ratio to Auto/16:9 and "
                "turn Widescreen Hack OFF.");
void WidescreenNeverCull()
{
    // never cull characters
    PatchInstruction_Conditional(0x8001DCF8, 0x7C001B78, 0x38000007);
    PatchInstruction_Conditional(0x806AA4E4, 0x38000002, 0x38000001);
    PatchInstruction_Conditional(0x806AB8B4, 0x38000000, 0x38000001);

    // never cull stadium hazards
    PatchInstruction_Conditional(0x806F7B7C, 0x881A0093, 0x38000003);
}

/*-----------------------------------------------------------
 Section 2: scale everything set through GXSetProjection
-----------------------------------------------------------*/

CGECKO(WidescreenProjection, .address = 0x800901F0,
                             .instruction = "lis r5, -13311");
void WidescreenProjection()
{
    // r5 still holds the GX state block pointer loaded at function entry
    READ_GAME_REG(u8*, gx, 5);

    // match: scale only the perspective (3D) cameras; the HUD is handled per component by Section 3
    if (inningSetting.rel == 5 && VAR_ADDRESS(u32, (u32)gx + 0x4D8) != 0)
        return;

    VAR_ADDRESS(float, (u32)gx + 0x4DC) *= WIDESCREEN_X_SCALE;  // m[0][0]
    VAR_ADDRESS(float, (u32)gx + 0x4E0) *= WIDESCREEN_X_SCALE;  // m[0][2]/m[0][3]
}

/*-----------------------------------------------------------
 Section 2b: the sprite/UI pipeline's projection (GXSetProjectionv)
-----------------------------------------------------------*/

CGECKO(WidescreenProjectionV, .address = 0x80090284,
                              .instruction = "lis r4, -13311");
void WidescreenProjectionV()
{
    // r5 = the GX state block (loaded at function entry)
    READ_GAME_REG(u8*, gx, 5);

    // match: leave the 2D layer stretched -- Section 3 handles the HUD
    if (inningSetting.rel == 5)
        return;

    // the 2D pipeline's screen-space perspective, and only it (m00 == 4.0 keeps save/restore from compounding)
    if (VAR_ADDRESS(u32, (u32)gx + 0x4D8) != 0)        // perspective?
        return;
    if (VAR_ADDRESS(u32, (u32)gx + 0x4DC) != 0x40800000)  // m00 == 4.0f?
        return;

    VAR_ADDRESS(float, (u32)gx + 0x4DC) = 4.0f * WIDESCREEN_X_SCALE;
}

/*-----------------------------------------------------------
 Section 3: un-stretch the match HUD and pin it to the sides
-----------------------------------------------------------*/

#define PIN_LEFT   -320
#define PIN_CENTER    0
#define PIN_RIGHT  +320

static const struct HudPin { u8 typeIndex; halfword textureID; short anchor; } HUD_PINS[] = {
    { 3, 179, PIN_LEFT   },
    { 3, 181, PIN_LEFT   },
    { 3, 180, PIN_RIGHT  },
    { 3, 182, PIN_RIGHT  },
    { 3, 183, PIN_CENTER },   // un-stretches in place, stays centered
};

CGECKO(WidescreenPinHud, .address = 0x8003553C,
                         .instruction = "lwz r0, 0(r28)");
void WidescreenPinHud()
{
    // r28 = the graphics object being drawn this iteration
    READ_GAME_REG(u8*, obj, 28);

    // match only: menu 2D content belongs in the centered 4:3 box
    if (inningSetting.rel != 5)
        return;

    if (*(obj + 0x70))          // is3D -- only flat HUD objects
        return;

    u8 type = *(obj + 0x66);
    halfword tex = VAR_ADDRESS(halfword, (u32)obj + 0x64);

    for (int i = 0; i < (int)LEN(HUD_PINS); i++)
    {
        if (HUD_PINS[i].typeIndex != type || HUD_PINS[i].textureID != tex)
            continue;

        // matrix row 0 at obj+0x0C: x' = s*x + a*(1-s)
        float* row0 = (float*)(obj + 0x0C);
        float anchor = (float)HUD_PINS[i].anchor;
        row0[0] *= WIDESCREEN_X_SCALE;
        row0[1] *= WIDESCREEN_X_SCALE;
        row0[2] *= WIDESCREEN_X_SCALE;
        row0[3] = row0[3] * WIDESCREEN_X_SCALE
                + anchor * (1.0f - WIDESCREEN_X_SCALE);
        return;
    }
}
