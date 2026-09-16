"""Build a curated gecko code list into a standalone, drop-in GYQE01.ini.

A "code list" is a manifest in codelists/ naming the FOLDERS whose codes belong
in one shipped bundle -- the ranked ruleset, the codes Rio itself applies, all of
them. Everything cgecko can build under those folders goes in, so adding a code
to "Gecko Codes/Ranked" is all it takes to put it in the ranked download; nobody
has to remember to update a list. A manifest's `exclude` is the escape hatch for
the handful of files that must stay out, and each one says why.

    python BuildCodeList.py                 # build every list into dist/
    python BuildCodeList.py ranked          # build one list
    python BuildCodeList.py --zip           # also package each list as a zip
    python BuildCodeList.py --check         # validate manifests, build nothing
    python BuildCodeList.py --write-scripts # regenerate the per-list launchers

Each list lands in dist/<list>/ as GYQE01.ini plus a README.txt naming the codes
and the commit it was built from. That ini REPLACES Rio's user ini; it is never
merged into one. Rio concatenates its Sys and user inis rather than letting one
override the other, so a code present in both is applied twice, which silently
overruns the gecko region and hangs the boot -- see BuildToRio.py for the
measurement.

Nothing here writes to a developer's own ini: every build passes cgecko's --ini,
so config.json is only read, never rewritten.
"""

import argparse
import fnmatch
import json
import os
import re
import shutil
import subprocess
import sys
import zipfile

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CGECKO     = os.path.join(SCRIPT_DIR, "CGecko", "cgecko.py")
LISTS_DIR  = os.path.join(SCRIPT_DIR, "codelists")
DIST_DIR   = os.path.join(SCRIPT_DIR, "dist")
INI_NAME   = "GYQE01.ini"          # the filename Rio looks for in GameSettings/
SOURCE_EXT = (".c", ".asm", ".ini")

# A gecko code line: two 8-digit hex words. Rio charges the game's heap 8 bytes
# for each one (it lowers ArenaHi by the size of the enabled list), so counting
# them is the only cost figure that matters for a shipped bundle.
CODE_LINE = re.compile(r"^[0-9A-Fa-f]{8} [0-9A-Fa-f]{8}$")


class ListError(Exception):
    """A manifest is malformed or names something that isn't there."""


# ==============================================================================
# MANIFESTS
# ==============================================================================
def load_manifest(path: str) -> dict:
    """Read one manifest. Folders are what a list is made of; `enabled` is the
    state every code in it ships with; `exclude` drops individual files."""
    slug = os.path.splitext(os.path.basename(path))[0]
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
    except json.JSONDecodeError as e:
        raise ListError(f"{slug}: not valid JSON ({e})")

    folders = data.get("folders")
    if not folders or not isinstance(folders, list):
        raise ListError(f'{slug}: manifest needs a "folders" list')

    return {
        "slug":        slug,
        "name":        data.get("name", slug),
        "description": data.get("description", ""),
        "install":     data.get("install", ""),
        "folders":     [f.replace("\\", "/").rstrip("/") for f in folders],
        # Whether the bundle's codes arrive switched on. A ruleset ships on; a
        # catalog of everything ships off, because half of it contradicts the
        # other half and enabling all of it at once would not boot.
        "enabled":     bool(data.get("enabled", True)),
        "exclude":     [e.replace("\\", "/") for e in data.get("exclude", [])],
    }


def find_manifests(names: list[str]) -> list[dict]:
    if not os.path.isdir(LISTS_DIR):
        raise ListError(f"no codelists folder at {LISTS_DIR}")
    available = sorted(f for f in os.listdir(LISTS_DIR) if f.endswith(".json"))
    if not available:
        raise ListError(f"no manifests in {LISTS_DIR}")

    if names:
        chosen = []
        for name in names:
            stem = os.path.splitext(os.path.basename(name))[0].lower()
            match = next((f for f in available
                          if os.path.splitext(f)[0].lower() == stem), None)
            if match is None:
                raise ListError(
                    f"no such code list: {name}\n  available: "
                    f"{', '.join(os.path.splitext(f)[0] for f in available)}")
            chosen.append(match)
    else:
        chosen = available

    return [load_manifest(os.path.join(LISTS_DIR, f)) for f in chosen]


def scan_folder(folder: str) -> list[str]:
    """Every gecko source under one folder, repo-relative and sorted -- the same
    sweep CGecko/build_all.py does, so a list holds exactly what the build check
    already compiles."""
    root = os.path.join(SCRIPT_DIR, folder)
    found = []
    for dirpath, dirnames, filenames in os.walk(root):
        # Skip hidden folders and nested checkouts (a decomp inside the tree is
        # not a pile of gecko codes).
        dirnames[:] = [d for d in dirnames
                       if not d.startswith(".")
                       and not os.path.exists(os.path.join(dirpath, d, ".git"))]
        for name in sorted(filenames):
            if name.endswith(".rewritten.c"):     # cgecko build leftover
                continue
            if name.endswith(SOURCE_EXT):
                full = os.path.join(dirpath, name)
                found.append(os.path.relpath(full, SCRIPT_DIR).replace("\\", "/"))
    return sorted(found)


def expand(manifest: dict) -> tuple[list[str], list[str]]:
    """Resolve a manifest's folders to source paths. Returns (sources, warnings);
    a warning is an exclude that matched nothing, which means the manifest has
    drifted from the tree it describes."""
    sources: list[str] = []
    for folder in manifest["folders"]:
        for rel in scan_folder(folder):
            if rel not in sources:
                sources.append(rel)

    warnings = []
    for pattern in manifest["exclude"]:
        hits = [s for s in sources
                if s.lower() == pattern.lower() or fnmatch.fnmatch(s.lower(),
                                                                   pattern.lower())]
        if not hits:
            warnings.append(f"exclude matches nothing (already gone?): {pattern}")
        sources = [s for s in sources if s not in hits]

    return sources, warnings


def validate(manifest: dict) -> list[str]:
    """A manifest must name folders that exist and produce at least one code.
    This is what --check (and CI) leans on when a folder gets renamed."""
    problems = []
    for folder in manifest["folders"]:
        if not os.path.isdir(os.path.join(SCRIPT_DIR, folder)):
            problems.append(f"missing folder: {folder}")
    if problems:
        raise ListError(manifest["slug"] + ":\n  " + "\n  ".join(problems))

    sources, warnings = expand(manifest)
    if not sources:
        raise ListError(f"{manifest['slug']}: folders hold no gecko sources")
    return warnings


# ==============================================================================
# BUILD
# ==============================================================================
def cgecko_supports_ini() -> bool:
    """The builder needs cgecko's --ini so it can write somewhere other than the
    developer's configured ini. An older submodule checkout won't have it."""
    result = subprocess.run([sys.executable, CGECKO, "--help"],
                            capture_output=True, text=True, errors="replace")
    return "--ini" in (result.stdout + result.stderr)


def ini_code_names(ini_path: str) -> list[str]:
    """The `$Name` headings currently in an ini's [Gecko] section."""
    if not os.path.isfile(ini_path):
        return []
    names, in_gecko = [], False
    with open(ini_path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            s = line.strip()
            if s.startswith("[") and s.endswith("]"):
                in_gecko = s == "[Gecko]"
            elif in_gecko and s.startswith("$"):
                names.append(s[1:].split("[")[0].strip())
    return names


def gecko_bytes(ini_path: str) -> int:
    """What the finished list costs the game's heap: Rio lowers ArenaHi by
    exactly the size of the enabled codes."""
    total = 0
    with open(ini_path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            if CODE_LINE.match(line.strip()):
                total += 8
    return total


def display_path(path: str) -> str:
    """Repo-relative when it can be (readable logs), absolute otherwise --
    os.path.relpath raises on Windows when --out points at another drive."""
    try:
        return os.path.relpath(path, SCRIPT_DIR)
    except ValueError:
        return path


def git_describe() -> str:
    """Short provenance for the README: what the bundle was built from."""
    try:
        sha = subprocess.run(["git", "rev-parse", "--short", "HEAD"],
                             cwd=SCRIPT_DIR, capture_output=True, text=True,
                             check=True).stdout.strip()
        dirty = subprocess.run(["git", "status", "--porcelain"],
                               cwd=SCRIPT_DIR, capture_output=True, text=True,
                               check=True).stdout.strip()
        return sha + (" (with uncommitted changes)" if dirty else "")
    except Exception:
        return "unknown"


def write_readme(manifest: dict, out_dir: str, built: list[str], size: int) -> None:
    lines = [manifest["name"], "=" * len(manifest["name"]), ""]
    if manifest["description"]:
        lines += [manifest["description"], ""]

    lines += ["INSTALL", "-------"]
    if manifest["install"]:
        # A list that isn't a player bundle -- the set Rio itself ships, say --
        # goes somewhere else entirely, so its manifest replaces this wholesale.
        lines += [manifest["install"], ""]
    else:
        lines += [
            "Replace the GYQE01.ini in Project Rio's GameSettings folder with the",
            "one in this folder. Do NOT merge the two files: Rio reads the shipped",
            "Sys ini AND the user ini and applies both, so a code kept in each is",
            "applied twice, which overruns the gecko region and hangs the boot.",
            "",
            "  Windows  %USERPROFILE%\\Documents\\Project Rio\\GameSettings\\",
            "  macOS    ~/Library/Application Support/Project Rio/GameSettings/",
            "  Linux    ~/.local/share/project-rio/GameSettings/",
            "",
        ]

    state = ("All of these are ON." if manifest["enabled"] else
             "All of these ship OFF -- turn on the ones you want in Rio's "
             "Gecko Codes list.")
    lines += ["CODES", "-----", state, ""]
    lines += [f"  {name}" for name in built]
    # Rio lowers ArenaHi by the size of the ENABLED codes, so a catalog that
    # ships switched off costs the heap nothing until you turn something on.
    cost = ("off the game's heap" if manifest["enabled"]
            else "if every one of them were enabled")
    lines += [
        "",
        f"{len(built)} code(s), {size:,} bytes of gecko list "
        f"({size / 1024:.1f} KB {cost}).",
        f"Built from ProjectRio-ASM {git_describe()}.",
        "",
    ]
    with open(os.path.join(out_dir, "README.txt"), "w",
              encoding="utf-8", newline="\r\n") as f:
        f.write("\n".join(lines))


def build_list(manifest: dict, dist: str, make_zip: bool) -> dict:
    """Build one manifest into dist/<slug>/GYQE01.ini. Returns a result dict."""
    out_dir  = os.path.join(dist, manifest["slug"])
    ini_path = os.path.join(out_dir, INI_NAME)
    sources, warnings = expand(manifest)

    # Start from nothing every time: a list is fully described by its manifest
    # and the tree, so a code deleted from a folder has to disappear here too.
    if os.path.isdir(out_dir):
        shutil.rmtree(out_dir)
    os.makedirs(out_dir, exist_ok=True)

    print("=" * 70)
    print(f"  {manifest['name']}  ->  {display_path(ini_path)}")
    print(f"  {len(sources)} source(s) from "
          f"{', '.join(manifest['folders'])}  "
          f"({'enabled' if manifest['enabled'] else 'disabled'} by default)")
    print("=" * 70)
    for warning in warnings:
        print(f"[LIST] WARNING: {manifest['slug']}: {warning}")

    env   = dict(os.environ, PYTHONUTF8="1")   # sources carry non-ASCII comments
    state = "--enabled" if manifest["enabled"] else "--disabled"
    built: list[str] = []
    failed: list[str] = []

    for rel in sources:
        print(f"\n[LIST] {rel}")
        before = ini_code_names(ini_path)
        result = subprocess.run(
            [sys.executable, CGECKO, state, "--no-launch", "--ini", ini_path,
             os.path.join(SCRIPT_DIR, rel)],
            stdin=subprocess.DEVNULL, env=env,
        )
        if result.returncode != 0:
            failed.append(rel)
            continue

        after = ini_code_names(ini_path)
        added = [n for n in after if n not in before]
        if not added:
            # cgecko replaces a same-named code in place, so two sources that
            # produce the same gecko name would silently collapse into one entry
            # and the list would ship short. Rename one, or exclude it.
            print(f"[LIST] ERROR: {rel} overwrote a code already in this list "
                  f"(same gecko name).\n"
                  f"       Rename one of them, or exclude it in "
                  f"codelists/{manifest['slug']}.json.")
            failed.append(rel)
            continue
        built.extend(added)

    size = gecko_bytes(ini_path) if os.path.isfile(ini_path) else 0
    if not failed:
        write_readme(manifest, out_dir, built, size)

    zip_path = None
    if make_zip and not failed:
        zip_path = os.path.join(dist, f"{manifest['slug']}.zip")
        with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as z:
            for fn in sorted(os.listdir(out_dir)):
                z.write(os.path.join(out_dir, fn), f"{manifest['slug']}/{fn}")
        print(f"\n[LIST] packaged {display_path(zip_path)}")

    return {"manifest": manifest, "built": built, "failed": failed,
            "warnings": warnings, "size": size, "ini": ini_path, "zip": zip_path}


def write_summary(results: list[dict], path: str) -> None:
    """A markdown table of what each list contains -- used as the release notes
    and as the Actions job summary, so the download page says what changed
    without anyone opening a zip."""
    lines = ["| Code list | Codes | Default state | Gecko list size |",
             "| --- | ---: | :--- | ---: |"]
    for r in results:
        state = "on" if r["manifest"]["enabled"] else "off"
        lines.append(f"| **{r['manifest']['name']}** | {len(r['built'])} | "
                     f"{state} | {r['size']:,} bytes |")
    lines.append("")
    for r in results:
        lines.append(f"### {r['manifest']['name']}")
        if r["manifest"]["description"]:
            lines += ["", r["manifest"]["description"], ""]
        for name in r["built"]:
            lines.append(f"- {name}")
        lines.append("")
    parent = os.path.dirname(os.path.abspath(path))
    if parent:
        os.makedirs(parent, exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    print(f"[LIST] wrote summary {path}")


# ==============================================================================
# LAUNCHER SCRIPTS
# ==============================================================================
BAT = """@echo off
REM Generated by BuildCodeList.py --write-scripts. Do not edit by hand.
python "%~dp0..\\BuildCodeList.py" {arg}
pause
"""

SH = """#!/bin/bash
# Generated by BuildCodeList.py --write-scripts. Do not edit by hand.
python3 "$(dirname "$0")/../BuildCodeList.py" {arg}
"""


def write_scripts() -> None:
    """Regenerate one double-clickable launcher per list, so building a bundle
    doesn't require remembering the command. Run this after adding a manifest."""
    scripts = os.path.join(SCRIPT_DIR, "Build Code Lists")
    os.makedirs(scripts, exist_ok=True)

    # "Build Every Code List" rather than "Build All": one of the lists is
    # itself called All, and two launchers a letter apart is a trap.
    wanted = {"Build Every Code List": "--zip"}
    for manifest in find_manifests([]):
        wanted[f"Build {manifest['name']}"] = f"--zip {manifest['slug']}"

    for stem, arg in sorted(wanted.items()):
        with open(os.path.join(scripts, stem + ".bat"), "w",
                  encoding="utf-8", newline="\r\n") as f:
            f.write(BAT.format(arg=arg))
        sh_path = os.path.join(scripts, stem + ".sh")
        with open(sh_path, "w", encoding="utf-8", newline="\n") as f:
            f.write(SH.format(arg=arg))
        os.chmod(sh_path, 0o755)
        print(f"[LIST] wrote {stem}.bat / .sh")

    # A manifest that was deleted shouldn't leave a launcher behind.
    keep = {stem + ext for stem in wanted for ext in (".bat", ".sh")}
    for fn in sorted(os.listdir(scripts)):
        if fn.endswith((".bat", ".sh")) and fn not in keep:
            os.remove(os.path.join(scripts, fn))
            print(f"[LIST] removed stale {fn}")


# ==============================================================================
# CLI
# ==============================================================================
def main() -> int:
    for stream in (sys.stdout, sys.stderr):
        reconfigure = getattr(stream, "reconfigure", None)
        if reconfigure:
            try:
                reconfigure(encoding="utf-8")
            except Exception:
                pass

    parser = argparse.ArgumentParser(
        description="Build curated gecko code lists into standalone Rio inis.")
    parser.add_argument("lists", nargs="*",
                        help="Manifest names to build (default: every manifest).")
    parser.add_argument("--out", default=DIST_DIR,
                        help="Output folder (default: dist/).")
    parser.add_argument("--zip", action="store_true",
                        help="Also package each list as <list>.zip.")
    parser.add_argument("--summary", metavar="FILE",
                        help="Write a markdown summary of what was built "
                             "(release notes, CI job summary).")
    parser.add_argument("--check", action="store_true",
                        help="Validate the manifests and build nothing.")
    parser.add_argument("--write-scripts", action="store_true",
                        help="Regenerate the per-list launcher scripts and exit.")
    args = parser.parse_args()

    try:
        if args.write_scripts:
            write_scripts()
            return 0

        manifests = find_manifests(args.lists)
        for manifest in manifests:
            for warning in validate(manifest):
                print(f"[LIST] WARNING: {manifest['slug']}: {warning}")
            sources, _ = expand(manifest)
            print(f"[LIST] {manifest['slug']}: {len(sources)} source(s)")
        if args.check:
            return 0

        if not cgecko_supports_ini():
            print("[LIST] ERROR: this CGecko checkout has no --ini flag, so a "
                  "list cannot be built\n"
                  "       without overwriting your own ini. Update the "
                  "submodule:\n"
                  "         git submodule update --remote CGecko",
                  file=sys.stderr)
            return 1
    except ListError as e:
        print(f"[LIST] ERROR: {e}", file=sys.stderr)
        return 1

    results = [build_list(m, args.out, args.zip) for m in manifests]

    if args.summary:
        write_summary(results, args.summary)

    print()
    print("=" * 70)
    for r in results:
        status = f"{len(r['failed'])} FAILED" if r["failed"] else "ok"
        print(f"  {r['manifest']['name']:<28} "
              f"{len(r['built']):>3} codes  {r['size']:>7,} bytes  {status}")
    print("=" * 70)

    broken = [rel for r in results for rel in r["failed"]]
    if broken:
        print("\n[FAILED]")
        for rel in broken:
            print(f"  {rel}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
