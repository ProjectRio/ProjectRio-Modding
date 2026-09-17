/*###########################################################
# Remove Slice - Frame 2 Stars
###########################################################*/
// Author: Roeming
#include "SliceAngles.h"

CGECKO(RemoveSliceFrame2Stars, .state = MSSB_GAME,
       .notes = "A slice is a non-line-drive star swing on the latest (frame 2) or earliest\n"
                "(frame 10) possible swing going straight up the middle.\n"
                "This code only turns the frame 2 slices into foul balls.");
void RemoveSliceFrame2Stars(void)
{
    static const SliceAngle fouls[] = {
        {0, 1,  2, -600},
        {1, 1,  2, -600},
        {2, 1,  2, -550},
        {2, 0, 10,  700},
    };
    if (SliceAnglesUntouched())
        ApplySliceAngles(fouls, sizeof(fouls) / sizeof(fouls[0]));
}
