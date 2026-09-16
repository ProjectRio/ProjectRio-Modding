# ProjectRio-ASM

Gecko codes and native-code mods (C/ASM, built via CGecko) for Mario Superstar
Baseball (MSSB), targeting the Project Rio netplay client.

## Writing mods

- **No excessive code comments.** Code should read from names and structure.
  If something genuinely needs explaining (a hardware quirk, a non-obvious
  invariant, why a workaround exists), that explanation belongs in `docs/`,
  not inline. A short comment is fine for a truly non-obvious one-liner; a
  paragraph never belongs in the source.
- **Report engine discoveries upstream.** Anything learned about how the game
  works while modding — symbol names, struct layouts, function behavior,
  addresses — should be pushed to the MSSB decomp repo (see
  `SyncFromDecomp.py` / `docs/decomp_integration_design.md`), not just kept
  as a local comment or note in this repo. The decomp is the shared source of
  truth; this repo should consume it, not fork knowledge away from it.
- **Prefer shared helpers over duplicated logic.** Anything likely to be
  reused across multiple mods — text rendering, scene/menu management, RAM
  patching patterns, etc. — should become a helper (see `Include/Rio/*.h`,
  e.g. `ScreenText.h`, `ScreenList.h`, `MenuScene.h`) rather than being
  re-implemented per mod.
- **Mod "notes" fields are for end users, not developers.** Keep them short,
  plain-language, and free of implementation detail, addresses, or jargon —
  the audience is non-technical players deciding whether to enable a mod, not
  someone maintaining the code.

## Repo layout

- `Gecko Codes/` — individual gecko codes (`Global/`, `Match/`, `Menu/`),
  written as `.c` (via CGECKO()/ASM() macros) or raw `.asm`.
- `RioModPack/` — the bundled mod pack shipped with Rio (Options Menu, Online
  Menu, Custom Music, etc.), gated by `CGECKO_GATE_ADDR`.
- `Decomp/` — git submodule, the MSSB decomp; source of truth for symbols,
  struct layouts, and game behavior. Fix mismatches there, not with local
  workarounds — see `SyncFromDecomp.py`.
- `Include/` — headers. `Include/Symbols/`, `Include/Rio/` etc. are
  synced/curated; `Include/Local` has been retired — no local ad hoc game
  headers, everything comes from the decomp sync.
- `codelists/`, `Build Code Lists/`, `BuildCodeList.py` — compiled/packaged
  gecko code lists.
- `docs/` — design docs and engine-knowledge writeups (this is where
  non-obvious explanations belong, per the rule above).

## Build & deploy

- `BuildToISO.py` bakes codes directly into the DOL — use this for anything
  that needs to be present at boot (e.g. `RioModPack`). `BuildToRio.py`
  targets the Rio gecko-ini path instead; don't use it for DOL-baked content.
- Rio **concatenates** its Sys ini and the user ini. A code present in both
  gets applied twice, which can hang the boot — only build/target the user
  ini.
- Build C gecko codes with `PYTHONUTF8=1` (CGecko needs it for UTF-8 string
  literals).
- Verify any match-boot-affecting change by actually launching Dolphin and
  polling `hasGameStarted_` (`0x80892AB5`) — never trust a savestate diff
  alone as proof a boot path works.
