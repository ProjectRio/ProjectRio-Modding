/*###########################################################
# Boot To Main Menu
###########################################################*/
// Author: LittleCoaks
// Boot walk, memory-card wizard and the two load methods: docs/boot_to_match.md
#include "Include/static/UnknownHomes_Static.h"

#define CARD_LOAD_GAME_WIZARD 0   // fire 0x8003F23C, game shows its box/prompts
#define CARD_LOAD_SILENT      1   // probe-gate + hide the box + auto-confirm

#define CARD_LOAD_METHOD  CARD_LOAD_SILENT

#define BOOT_TO_MENU_SITE   0x8063F964
#define LI_R3_5             0x38600005      // li r3, 5  (screen 5 = main menu)

#define cardLoadModeByte    VAR_ADDRESS(u8, 0x80366177)   // set before the check
static inline void loadMemoryCardSlotA(void) { ((void(*)(void))0x8003F23C)(); }

// SDK CARDProbeEx(chan, *memSize, *sectorSize): 0 = ready, -1 = busy, -3 = no card
#define CARD_RESULT_READY   0
#define CARD_RESULT_BUSY    (-1)
static inline int cardProbeExSlotA(void)
{
    int memSize, sectorSize;
    return ((int(*)(int, int*, int*))0x80086144)(0, &memSize, &sectorSize);
}

// the wizard's dialog fields in the card-check struct at 0x803C50E8
#define dlgActive           VAR_ADDRESS(u8, 0x803C5126)   // 0x803C50E8 + 0x3e
#define dlgResponseA        VAR_ADDRESS(u8, 0x803C512C)   // 0x803C50E8 + 0x44
#define dlgResponseB        VAR_ADDRESS(u8, 0x803C512D)   // 0x803C50E8 + 0x45
#define DLG_CONFIRM         3        // "proceed" branch of the wizard's poll

#define SUPPRESS_FRAMES     600      // ~10 s at 60 fps

// persistent "already loaded" latch; must NOT be 0x803C50E8 (the card task reads that as an abort flag)
#define g_loaded            VAR_ADDRESS(u8, 0x802EC01B)

// per-menu-load probe phase: 0 = probe, 1 = suppression window, 2 = idle
#define g_phase             VAR_ADDRESS(u8, 0x802EC01E)
#define g_frames            VAR_ADDRESS(halfword, 0x802EC01C)

// No .state: has to see rel 0 (force the boot walk) AND rel 4 (drive the card load).
CGECKO(BootToMainMenu,
       .notes = "Boots directly to the main menu, skipping the intro and title screen.");
void BootToMainMenu()
{
    if (inningSetting.rel == 0)
    {
        PatchInstruction(BOOT_TO_MENU_SITE, LI_R3_5);
        g_phase = 0;
    }

    if (inningSetting.rel != 4)
        return;

    if (g_loaded)
        return;

#if CARD_LOAD_METHOD == CARD_LOAD_GAME_WIZARD
    if (g_phase == 0)
    {
        g_phase  = 2;
        g_loaded = 1;
        cardLoadModeByte = 1;
        loadMemoryCardSlotA();
    }
#else
    if (g_phase == 0)
    {
        int probe = cardProbeExSlotA();
        if (probe == CARD_RESULT_BUSY)
            return;                          // still detecting -- try next frame
        if (probe == CARD_RESULT_READY)
        {
            cardLoadModeByte = 1;
            loadMemoryCardSlotA();
            g_frames = 0;
            g_phase  = 1;
        }
        else
        {
            // no card this menu load; g_loaded stays 0 so the next menu load re-probes
            g_phase = 2;
        }
    }
    else if (g_phase == 1)
    {
        // hide the wizard's box and auto-confirm its prompts while it loads or creates
        dlgActive    = 0;
        dlgResponseA = 1;
        dlgResponseB = DLG_CONFIRM;
        if (++g_frames >= SUPPRESS_FRAMES)
        {
            g_phase  = 2;
            g_loaded = 1;   // load done -- never load again this session
        }
    }
#endif
}
