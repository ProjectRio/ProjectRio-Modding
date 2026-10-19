// The lefty contact-zone mirror in game.rel and why it is lopsided: docs/import_balance.md
#pragma once

#define SKIP_MIRROR_FOR_RIGHTY   0x80651158
#define MIRROR_CONTACT_FOR_LEFTY 0x8065115C

#define BEQ_SKIP_MIRROR  0x41820008
#define FNEG_F9          0xFD204850
#define NOP              0x60000000
