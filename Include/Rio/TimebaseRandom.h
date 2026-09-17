#ifndef RIO_TIMEBASE_RANDOM_H
#define RIO_TIMEBASE_RANDOM_H

/* UnclePunch's inline random for ASM() bodies: r15 = 0..range-1, seeded from
   the CPU timebase. Uses r14-r16; end the body with TIMEBASE_RANDOM_CLEAR.
   See docs/import_match_gameplay.md. */
#define TIMEBASE_RANDOM_R15(range)        \
    "li     15, " range "             \n" \
    "lis    16, 3                     \n" \
    "addi   14, 16, 0x43FD            \n" \
    "mftb   16                        \n" \
    "mullw  16, 16, 14                \n" \
    "addis  16, 16, 0x27              \n" \
    "addi   14, 16, -0x613D           \n" \
    "rlwinm 14, 14, 16, 16, 31        \n" \
    "mullw  14, 15, 14                \n" \
    "srawi  15, 14, 16                \n" \
    "addze  15, 15                    \n"

#define TIMEBASE_RANDOM_CLEAR             \
    "li     14, 0                     \n" \
    "li     15, 0                     \n" \
    "li     16, 0                     \n"

#endif
