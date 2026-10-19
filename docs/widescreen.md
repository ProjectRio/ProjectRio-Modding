# Widescreen: how the game-side 16:9 works

Used by: `Gecko Codes/Global/Widescreen.c`.

## The idea

Everything the game draws is scaled to 3/4 width on the EFB, so that Dolphin's
"Force 16:9" stretch (4/3) cancels it out exactly. For 3D that means a wider
FOV filling the whole screen; for finite 2D content it means the layer renders
pixel-identical to 4:3, centered, with the extra screen width to its sides, so
menus simply don't stretch. `WIDESCREEN_X_SCALE = (4/3) / (16/9) = 0.75`; for
another display use `(4.0/3.0) / aspect`, e.g. 21:9 -> 0.5714.

Two hooks cover every projection upload. Neither carries a `.state` gate: the
projection hooks must be live from the first boot frame so menus never
stretch; each section gates itself on `rel` where it matters.

### 1. GXSetProjection (0x8009019C), hook at 0x800901F0

Used for the 3D cameras and the ortho 2D layers. It copies the six meaningful
matrix entries into the GX state block before writing the XF registers:

| field    | meaning                                    |
| -------- | ------------------------------------------ |
| gx+0x4D8 | projection type (0 = perspective, 1 = ortho) |
| gx+0x4DC | m[0][0], x scale                           |
| gx+0x4E0 | m[0][2] (persp) / m[0][3] (ortho)          |

The hook runs after the copy (r5 still holds the GX state block pointer loaded
at function entry; the overwritten instruction is `lis r5, -13311`) and scales
those two entries by 3/4 in place. The game's own matrices (e.g. the
`g_pCamera` matrix at `0x803C64EC` that `setScissorAndProjection` re-sends
every frame) are never touched, and this path is never used to restore a saved
projection (`GXGetProjectionv` output can only be fed back through
`GXSetProjectionv`), so nothing compounds frame over frame.

In the match (`rel == 5`) only perspective cameras are scaled; the ortho 2D
layers stretch with the display like the rest of the match 2D, and the HUD is
handled per component (section 3).

### 2. GXSetProjectionv (0x80090240), hook at 0x80090284

The sprite/UI pipeline sets its screen-space projection through this from a
stored 7-float array (`lwz r3, -31744(r13)` at `0x800B248C`): a y-flipped
perspective with m[0][0] = 4.0 that maps x = +-320 to the screen edges. That
one constant drives all 2D in every scene: menus, the match HUD, text.

Setv is also the restore path for projections saved with `GXGetProjectionv`,
so it must not scale blindly (a restore of an already-scaled projection would
compound). The hook matches the 2D pipeline's signature exactly (perspective
and m[0][0] bits == `0x40800000`, 4.0f) and *replaces* it with 4.0 * 3/4 =
3.0. A restored value is 3.0, which no longer matches: idempotent by
construction.

In the match this hook deliberately stands down: the match 2D layer is left to
stretch, and section 3 re-shapes each HUD component individually. This is the
combination verified on real hardware-path testing; un-stretching the whole
match layer and translating the HUD outward looked equivalent on paper but did
not move the HUD in practice.

### 3. Un-stretch the match HUD and pin it to the sides, hook at 0x8003553C

Every 2D graphics object (pool: 864 x 0xC0 at `0x8039C3E0`) is drawn by the
per-frame object pass (`0x80035240`-`0x80035670` in main.dol). Each frame it
rebuilds the object's matrix at obj+0x0C (transformedPos = position -
texsize/2, PSMTXTrans, concat with obj+0x78), then at `0x8003553C` composes it
with a per-class base matrix and hands the result to the sprite drawer
(`0x8000D838`). Injecting there (overwritten instruction `lwz r0, 0(r28)`, r28
= the object) applies the correction to a freshly built matrix every frame:
nothing compounds, nothing to restore.

Each HUD component is one object drawing a full 640x448 canvas (matrix output
space: screen pixels centered at 0, x in [-320, 320]). The correction squeezes
x by 3/4 about its anchor edge:

    x' = 0.75*x + anchor*(1 - 0.75)

Under the 16:9 display stretch (4/3) that cancels to x' = x at the anchor: the
component keeps its shape, left boxes stay on the left edge, right boxes on
the right edge, center popups un-stretch in place. Only row 0 of the matrix
(obj+0x0C: [m00 m01 m02 m03]) changes: scale m00..m02, re-aim m03.

Object fields: obj+0x70 = is3D (only flat objects are touched), obj+0x66 =
typeIndex (picks the sprite bank at `0x803C4BE0`), obj+0x64 = textureID.
Known in-game HUD components (game REL, from live object dumps; the game
groups the HUD by screen side, tex 179/180 are the same layout mirrored):

| type | tex | component                          | anchor |
| ---- | --- | ---------------------------------- | ------ |
| 3    | 179 | left-side HUD group (score/inning) | left   |
| 3    | 181 | small left-side group              | left   |
| 3    | 180 | right-side HUD group (batter info) | right  |
| 3    | 182 | small right-side group             | right  |
| 3    | 183 | center popups (contact "!!" etc.)  | center |

Components not in the table (fades, unknown 2D objects) are left alone:
stretched, but never broken. To fix another, find its typeIndex and textureID
and add a row with the edge it lives on.

## Never-cull patches

The game marks actors/props invisible when they leave the original 4:3
frustum, so with the wider FOV they would pop in and out at the edges. Four
instruction patches (the community "never cull" codes) force the flags:

| address      | original             | patched   | what                                     |
| ------------ | -------------------- | --------- | ---------------------------------------- |
| `0x8001DCF8` | `or r0,r0,r3` (7C001B78) | `li r0,7` (38000007) | actor per-camera visibility bits (main.dol, characters) |
| `0x806AA4E4` | `li r0,2` (38000002) | `li r0,1` | fielder visibility (game REL, animateDefence) |
| `0x806AB8B4` | `li r0,0` (38000000) | `li r0,1` | batter/runner visibility (game REL, animateOffence) |
| `0x806F7B7C` | `lbz r0,0x93(r26)` (881A0093) | `li r0,3` (38000003) | stadium hazard draw state (game REL, loadStadiumObjectVisuals) |

The REL sites are re-applied from a per-frame code with an original-instruction
guard (`PatchInstruction_Conditional`), because scene/state transitions reload
the REL and erase the patch, and the guard keeps the code from scribbling on a
different REL's code.

## Notes on the code structure

`.notes` sits on the first of the file's four hooks; they all share one option
so the notes table takes the first that declares any.
