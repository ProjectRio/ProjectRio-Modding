/*###########################################################
# Hover Over Unused CSS Squares
###########################################################*/
// Author: LittleCoaks

#define SKIP_EMPTY_SQUARE_RIGHT  0x80050A54
#define SKIP_EMPTY_SQUARE_LEFT   0x80050AFC
#define NOP 0x60000000

CGECKO(HoverOverUnusedCSSSquares,
       .notes = "Lets the cursor rest on the unused squares of the character select screen.\n"
                "Just for fun: selecting an unused character will likely crash the game.");
void HoverOverUnusedCSSSquares(void)
{
    PatchInstruction(SKIP_EMPTY_SQUARE_RIGHT, NOP);   // bge 0x80050A78
    PatchInstruction(SKIP_EMPTY_SQUARE_LEFT,  NOP);   // bge 0x80050B20
}
