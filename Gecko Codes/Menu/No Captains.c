/*###########################################################
# No Captains
###########################################################*/
// Author: Mori

// menus.rel instructions, re-patched per frame while the menu REL is resident
#define CAPTAIN_ID_LOAD        0x806527E0
#define CAPTAIN_CHOSEN_STORE   0x8064FDAC

CGECKO(NoCaptains, .state = MSSB_MENU,
       .notes = "Lets you draft a team without picking a captain.");
void NoCaptains(void)
{
    PatchInstruction_Conditional(CAPTAIN_ID_LOAD,      0x80050000, 0x380000FF);   // lwz r0, 0(r5)     -> li r0, 0xFF
    PatchInstruction_Conditional(CAPTAIN_CHOSEN_STORE, 0x98060052, 0x7F40D378);   // stb r0, 0x52(r6)  -> mr r0, r26
}
