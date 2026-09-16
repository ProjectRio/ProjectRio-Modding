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

A *code list* is a folder of codes packaged as one drop-in `GYQE01.ini`. Each
list is a manifest in `codelists/` naming the folders it covers:

| List | Folder | Ships |
| --- | --- | --- |
| `ranked` | `Gecko Codes/Ranked` | on |
| `rio-built-in` | `Gecko Codes/Rio Built-in` | on |
| `all` | `Gecko Codes` (everything) | off -- a catalog to pick from |

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

Every push to `main` builds all of them in CI and republishes them as the
rolling **codes-latest** release, so the download always matches `main`. Pull
requests build the lists too, which is what catches a folder somebody renamed.

A packaged ini **replaces** Rio's user ini; it is never merged into one. Rio
applies every ini it reads rather than letting one override another, so a code
that ends up in the file twice overruns the gecko region and hangs the boot.

### Happy modding!
