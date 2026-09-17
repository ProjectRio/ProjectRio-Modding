/*###########################################################
# Characters Are Big
###########################################################*/
// Author: Roeming, PeacockSlayer
#define CHARACTER_SCALE 2.5
#include "Gecko Codes/Balance/CharacterScale.h"

CGECKO(CharactersAreBig,
       .notes = "All characters are 2.5x their normal size.");
void CharactersAreBig(void)
{
    ApplyCharacterScale();
}
