# Swing frame counting, and skipping frame 1

Used by: `Gecko Codes/Match/Skip First Swing Frame.c`.

## How a swing counts

`ifSwing` (game.rel `0x806522F0`) runs every frame while `g_Batter.swingInd`
is set and does `framesSinceStartOfSwing++` before any of the contact / sound /
animation logic looks at the counter. It is 0 on the frame the swing starts,
so the first pass makes it 1, the next 2, and so on. The hittable window
(`hittableFrameInd`) opens at 2: slap swings are hittable on frames 2-10,
charge swings on 3-9, so frame 1 can never make contact.

## The mod

On that first pass, add 2 instead of 1: the counter goes 0 -> 2 and the swing
lands straight in the hittable window. Every later pass is the plain +1, so
the rest of the swing (sound on frame 5/6, animation end at frameDelay) is
unchanged and merely arrives a frame sooner.

### Hook 1: the increment (0x80652374)

Injected on the `addi r0, r5, 1` itself (r5 = old counter, r0 = new one,
stored two instructions later). ASM rather than C because the result has to
come back in r0, which the C2-in-C wrapper cannot deliver (it clobbers r0 and
reloads it with the saved LR on the way out). cr0 is free: nothing between
this and the next compare reads it, and the call two instructions down would
trash it anyway. r11/r12 are the scratch.

Do not use r3/r4 despite the `addi r4, r4, 0x910` / `addi r3, r3, 0x5D38`
right below: those only complete `lis` halves loaded two instructions above
the hook, so writing r3/r4 turns `g_Batter` into 0x910 and the
`stb r0, 0x9B(r4)` below into a wild store into the OS area.

The hook also clears the swing latch (below), because this is the one place
that knows a new swing has begun.

### Why a latch and not a counter value

Everything that should happen once per swing used to trigger on
`framesSinceStartOfSwing == 1`; the mod's obvious replacement, `== 2`, is not
actually once. When the ball is hit on the swing's first frame the play moves
on and `atBat_batter` stops being called, so `ifSwing` never runs again and the
counter sits at 2 with `swingInd` still set for the rest of the play. Every
`== 2` test then fires on every frame: measured, the swing animation re-armed
and restarted continuously, the actor flickering between 0x4D and its
follow-through 0x50. Vanilla never has this problem because the value it
freezes at is never 1.

So the two animation edges are driven off a one-shot latch instead:

| item                 | value                                          |
| -------------------- | ---------------------------------------------- |
| SWING_LATCH          | `0x802EAF90` (claimed RAM), reached by `lis 0x802F` / `-0x5070` |
| LATCH_REACTION_HELD  | bit 0: hit reaction already held this swing    |
| LATCH_ANIM_ARMED     | bit 1: swing animation already armed this swing |

### Hook 2: letting the swing animation happen at all (0x806A2D18)

`simulate1FrameOfTheGame` (`0x80699D34`) runs `baseballMatchSimulation`
(which contains `ifSwing`) before `matchAnimations` -> `batterAnimations`. So
on the swing's first frame the counter already reads 2 when the animation code
looks at it, and that is now also the first frame contact can happen. On the
frame the ball is hit, `batterAnimations` never gets as far as the swing: at
`0x806A2D18` it tests the play state, takes the hit-reaction branch (a switch
on `g_Batter.hitTrajectory` that plays 0x63 / 0x64 / 0x51 ...) and returns.
Vanilla survives because its swing animation was set a frame earlier; with the
mod the two land on the same frame and the swing loses. Measured on a real
frame-2 hit: the batter goes straight from pre-pitch animation 0x56 to hit
reaction 0x63 with no swing in between.

So for one frame per swing the hit reaction is held back and the ordinary
animation path runs instead, which reaches the swing block, arms, and starts
the swing. The next frame the latch is set and the reaction goes ahead.
`0x806A2D18` is `cmplwi r0, 2`, feeding the `blt` to the ordinary path on the
next instruction, so forcing that compare is all it takes. The hook reads
`g_Batter.swingInd` (`0x8089099D`) and `framesSinceStartOfSwing`
(`0x80890976`, lha). r0 is untouched; r11/r12 do not appear anywhere in
`batterAnimations`.

### Hook 3: arming the swing animation (0x806A3688)

`batterAnimations+0xAEC` is the `cmpwi r0, 1` that decides whether to work out
the animation's start delay and clear its "already playing" flag. The block
already sits inside `if (g_Batter.swingInd)`, so the counter value was only
ever the game's way of saying "first frame of this swing". The latch says that
properly and cannot get stuck. cr0 is left as the original compare would have
left it: equal = run the block.

### The replay / camera edges (per-frame patches)

Three more `framesSinceStartOfSwing == 1` tests exist in game.rel (found by
walking every halfword load at `g_Batter+0x66` whose base resolves to
`g_Batter`):

| address      | function                               | purpose                        |
| ------------ | -------------------------------------- | ------------------------------ |
| `0x80654504` | `fn_3_15458+0x18`                      | camera: stamps the swing's start frame for the replay, raises the camera's swing flag |
| `0x80654610` | `assignFrameCountersToAPointer+0x58`   | replay-recorder stamp (halfword at +0xA76, flag at +0xAB5) |
| `0x80654B70` | `fn_3_15A98+0x44`                      | replay flag                    |

These are pure bookkeeping, so they move from `cmpwi r0,1` (`2C000001`) to
`cmpwi r0,2` (`2C000002`) rather than getting latch bits. The frozen-counter
case applies to them too: on a swing that connects on the very first hittable
frame they re-stamp every frame for the rest of the play. Invisible in play,
only replay bookkeeping; the known rough edge. Patched per frame because these
are game.rel instructions and have to be re-applied every load;
`PatchInstruction_Conditional` makes ON and OFF the same call with the last two
arguments swapped, both idempotent.
