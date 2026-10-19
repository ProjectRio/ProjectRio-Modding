# Control Stick Overrides DPad

Used by: `Gecko Codes/Rio Built-in/Control Stick Overrides DPad.c` (hook at
`0x800A5A00`, DOL, always).

PADRead copies each channel's SI input buffer into its per-port state:

```
800a59f4  lwz  r0, 4(r4)          ; SI inbuf high word: buttons | stickX | stickY
800a59f8  add  r5, r28, r3
800a59fc  stw  r0, 0x1c0(r5)      ; <- the .asm hooked here and edited r0
800a5a00  slwi r0, r29, 2         ; r0 is dead: recomputed from the port index
```

The word is `BBBB XX YY`: the button halfword on top (D-pad bits
`0x000F0000`), then stick X and stick Y as unsigned bytes centred on 0x80.
When either axis is at or beyond 0x52 / 0xAE the D-pad bits are cleared.

The C version hooks one instruction later than the .asm did. At `0x800A59FC`
the value being stored is in r0, which the C wrapper clobbers, and re-reading
the SI register would be a second hardware read; at `0x800A5A00` the word is
already in memory at `r5 + 0x1C0`, r0 is dead (the re-issued `slwi` defines
it) and cr0 is not read again before the next compare at `0x800A5A28`.
