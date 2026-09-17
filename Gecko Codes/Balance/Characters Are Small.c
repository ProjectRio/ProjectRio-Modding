/*###########################################################
# Characters Are Small
###########################################################*/
// Author: Roeming, PeacockSlayer
#define CHARACTER_SCALE 0.25
#include "Gecko Codes/Balance/CharacterScale.h"

CGECKO(CharactersAreSmall,
       .notes = "All characters are a quarter of their normal size.");
void CharactersAreSmall(void)
{
    ApplyCharacterScale();
}
