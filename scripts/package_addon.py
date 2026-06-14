#!/usr/bin/env python3
"""Assemble the distributable addon and zip it for a GitHub release.

Copies the addon files plus our compiled extension binaries
(libdiscord_social_sdk.*) from one or more --bin directories. It deliberately
EXCLUDES Discord's runtime libraries (libdiscord_partner_sdk.* /
discord_partner_sdk.*) — those are not redistributed; users add them from
their own Discord Developer Portal download.

Usage:
    python scripts/package_addon.py --version 0.1.0 \
        --bin path/to/artifacts/bin-macos-universal \
        --bin path/to/artifacts/bin-linux-x86_64 ...
"""
import argparse
import glob
import os
import shutil
import zipfile

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ADDON_SRC = os.path.join(REPO_ROOT, "demo", "addons", "discord_social_sdk")

# Files copied verbatim into the packaged addon (everything except bin/).
ADDON_FILES = [
    "plugin.cfg",
    "plugin.gd",
    "discord_autoload.gd",
    "README.md",
    "discord_social_sdk.gdextension",
]

# Our extension binaries; anything not matching is left out (notably Discord's
# runtime libs).
EXT_GLOBS = [
    "libdiscord_social_sdk.*.dylib",
    "libdiscord_social_sdk.*.so",
    "libdiscord_social_sdk.*.dll",
]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--version", required=True)
    p.add_argument("--bin", action="append", default=[], help="dir with built extension binaries")
    p.add_argument("--out", default=os.path.join(REPO_ROOT, "dist"))
    args = p.parse_args()

    # If no --bin given, default to the local demo bin dir.
    bin_dirs = args.bin or [os.path.join(ADDON_SRC, "bin")]

    staging = os.path.join(args.out, "discord_social_sdk")
    if os.path.isdir(staging):
        shutil.rmtree(staging)
    os.makedirs(os.path.join(staging, "bin"))

    for f in ADDON_FILES:
        src = os.path.join(ADDON_SRC, f)
        if os.path.isfile(src):
            shutil.copy2(src, os.path.join(staging, f))
        else:
            print(f"  warning: addon file missing: {f}")

    copied = 0
    for bin_dir in bin_dirs:
        for pattern in EXT_GLOBS:
            for path in glob.glob(os.path.join(bin_dir, "**", pattern), recursive=True):
                shutil.copy2(path, os.path.join(staging, "bin", os.path.basename(path)))
                copied += 1
                print(f"  + {os.path.basename(path)}")
    print(f"Collected {copied} extension binaries.")

    zip_path = os.path.join(args.out, f"discord_social_sdk-{args.version}.zip")
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as z:
        for root, _dirs, files in os.walk(staging):
            for name in files:
                full = os.path.join(root, name)
                # Archive paths start with addons/discord_social_sdk/...
                arc = os.path.join("addons", os.path.relpath(full, args.out))
                z.write(full, arc)
    print(f"Wrote {zip_path}")


if __name__ == "__main__":
    main()
