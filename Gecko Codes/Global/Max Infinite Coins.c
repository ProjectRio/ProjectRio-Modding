/*###########################################################
# Max Infinite Coins
###########################################################*/
// Author: Codejunkies
#include "Include/types.h"

#define CoinTotal VAR_ADDRESS(u16, 0x8036600C)

CGECKO(MaxInfiniteCoins, .notes = "Your coin total is always 999.");
void MaxInfiniteCoins(void)
{
    CoinTotal = 999;
}
