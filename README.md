# Project Rio ASM/C

A repo for all of the gecko codes made by Project Rio. Utilizes [CGecko](https://github.com/LittleCoaks/CGecko) to maximize convenience and efficiency for modders.
**Must read the CGecko README/Wiki before beginning your mods!**

## Setup
1. Clone this repo with:

```bash
git clone --recurse-submodules https://github.com/ProjectRio/ProjectRio-ASM
```

2. Pull submodules if CGecko is not already present

```bash
git submodule update --init --recursive
```

3. Copy/Paste `config.template.json` from the CGecko folder to the ProjectRio-ASM root directory. Change filename to `config.json` and include your configurations.

4. Setup and learn [CGecko](https://github.com/LittleCoaks/CGecko)

## Tips
- Check out existing codes here in the Gecko Codes folder to see how other mods are typically written
- Update the submodules every so often to make sure you're on the latest version of CGecko

## Code lists

A *code list* is a set of codes packaged as one drop-in `GYQE01.ini`. Each list
is a manifest in `codelists/` naming the folders -- or the individual files --
it covers:

| List | Covers | Ships |
| --- | --- | --- |
| `ranked` | `Gecko Codes/Ranked` | on |
| `rio-built-in` | `Gecko Codes/Rio Built-in` | on |
| `all` | `Gecko Codes` (everything) | off -- a catalog to pick from |
| `rio-server` | the files named in the manifest | off -- what the Rio server offers players |

Everything cgecko can build under those folders goes in, so **adding a code to
`Gecko Codes/Ranked` puts it in the ranked download** with no list to update.

```json
{
    "name": "Ranked",
    "folders": ["Gecko Codes/Ranked"],
    "enabled": true,
    "exclude": ["Gecko Codes/Ranked/superseded.asm"]
}
```

A list that is a deliberate selection names `files` instead of (or as well as)
`folders`. `rio-server` works that way: a new code is offered to players only
once its path is added to `codelists/rio-server.json`, never just because it
landed in a folder. A path that no longer exists fails `--check`, so renaming a
code cannot silently drop it from the server.

```json
{
    "name": "Rio Server",
    "files": ["Gecko Codes/Match/Disable Replays.c", "Gecko Codes/Menu/No Captains.c"],
    "enabled": false
}
```

Besides `GYQE01.ini`, every list is written as `gecko_codes.json` (name, authors,
code lines and description per code, for a program to read) and
`gecko_codes.txt` (the plain `Name [Authors]` / code / description layout the
in-service list has always used).

`exclude` is for the few files that must stay out -- it exists because cgecko
keys an ini entry by name, so two sources with the same code name would silently
collapse into one. The builder fails the list when that happens and names both.

Build them with `BuildCodeList.py`, or double-click a launcher in
`Build Code Lists/`:

```bash
python BuildCodeList.py            # every list, into dist/<list>/GYQE01.ini
python BuildCodeList.py ranked     # just one
python BuildCodeList.py --check    # validate the manifests, build nothing
```

This never touches the ini in your `config.json` -- each list is built to its
own file via cgecko's `--ini`. After adding or renaming a manifest, run
`python BuildCodeList.py --write-scripts` to refresh the launchers.

CI (`.github/workflows/build.yml`) runs on every push and pull request: it
compiles every code under `Gecko Codes/` plus the RioModPack bundle, then builds
the lists and uploads two artifacts -- **all-codes** (every code) and
**rio-server-codes** (only the selection in `rio-server.json`). On `main` the
same files are republished as the rolling **codes-latest** release, so the
server has a stable URL that always matches `main`:

```
https://github.com/ProjectRio/ProjectRio-Modding/releases/download/codes-latest/rio-server.json
```

A packaged ini **replaces** Rio's user ini; it is never merged into one. Rio
applies every ini it reads rather than letting one override another, so a code
that ends up in the file twice overruns the gecko region and hangs the boot.

### Happy modding!
