# Petey's unused bat

`Gecko Codes/Match/Petey Uses His Bat.c`

## The asset

Petey (slot 38) has a 60-triangle bat (mesh `L_bat`, textured from his body set)
in every hand slot: `CharacterHandFiles` (0x800F5D98) entries 228..233 point at
ZZZZ.dat 0x188D9800 (left, hands 0/2) and 0x188DA800 (right, hands 3/5), and the
ARAM copies are `AramCharacterFiles` items 320..323. The game loads them like any
character's hands (`animateModelArmsGlovesBats` -> `loadCharacterHandItem`
0x80021CA4, DVD or ARAM path) but never draws them.

## How hands get drawn

A hand is drawn by pointing a body bone's display object (`bone+0x14`) at the
hand actor's geometry. `setHandModelAttached(player, slot, attach)` 0x8001D180
does it: the body bone is `player->boneIndex[slotBone[slot]]` (`player+0x162`,
`handAttachBoneIDs` 0x803CB7B4 = 19, 25, 20, 26, 74, 75, 60, 61, 64), the geometry
is the last bone with a display object in the left (`record+0x60`, even slot) or
right (`record+0xC4`, odd slot) hand actor, and `attach == 0` clears the
pointer instead.

## Why Petey's never shows

Petey, Monty Mole and the Goombas go through `initSpecialHandBones` 0x8004BD6C / `updateSpecialBatAttach` 0x8004BAEC
instead of the wrist attach. For Petey `updateSpecialBatAttach`:

1. always detaches slots 0/1 (wrist bones 19/25);
2. picks slots from the `specialBatAttachSlots` 0x800FC388 (Petey = 2, 3: bones 20/26, the
   tips of his leaf arms; Monty = 4, 5);
3. attaches a slot only while the player holds the bat (`+0x274` or `+0x278`)
   AND `getHandTrackFlags(handDisplay)` 0x80024D2C returns bit 0x20000000, the top nibble of the
   current keyframe in the hand's own animation track.

Petey's hand tracks never carry that bit, so step 3 never fires. The code ORs
the bit into the result at 0x8004BCEC (the `rlwinm.` that tests it) for Petey on
loop pass r29 == 0. The loop pairs pass 0 with the right leaf for right-handed
batters and the left leaf for lefties (`r8 = batting hand == 0`), so the bat is
in the batting-side leaf. Forcing both passes draws a bat on each leaf.

Verified live 2026-09-22 (Rio, forced-boot match with Petey at bat): stock
shows no bat; with the code the bat is on the leaf in the stance and through
the swing. Fielding and pitching were not checked.
