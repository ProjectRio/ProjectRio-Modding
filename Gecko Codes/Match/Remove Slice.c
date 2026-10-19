/*###########################################################
# Remove Slice
###########################################################*/
// Author: Roeming
#include "SliceAngles.h"

CGECKO(RemoveSlice, .state = MSSB_GAME,
       .notes = "A slice is a non-line-drive star swing on the latest (frame 2) or earliest\n"
                "(frame 10) possible swing going straight up the middle.\n"
                "This code turns those slices into foul balls.");
void RemoveSlice(void)
{
    static const SliceAngle fouls[] = {
        {0, 1,  2, -600}, {0, 1, 10, 600},
        {1, 1,  2, -600}, {1, 1, 10, 700},
        {2, 1,  2, -550}, {2, 1, 10, 650},
        {2, 0, 10,  700},
    };
    if (SliceAnglesUntouched())
        ApplySliceAngles(fouls, sizeof(fouls) / sizeof(fouls[0]));
}
