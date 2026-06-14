#!/usr/bin/env python3
"""Install the Discord Social SDK into this repository.

The Discord Social SDK is downloaded per-application from the Discord Developer
Portal and is not redistributable, so it is not committed here. This script
copies an extracted SDK into `thirdparty/discord_social_sdk/` and copies the
matching runtime library into the addon's `bin/` directory.

Usage:
    python scripts/setup_sdk.py /path/to/extracted/discord_social_sdk
    python scripts/setup_sdk.py --merge-universal ARM64_PATH X86_64_PATH

The source path is the folder that directly contains `include/`, `lib/` and
`bin/` (typically named `discord_social_sdk`).
"""
import argparse
import os
import platform
import shutil
import subprocess
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
THIRDPARTY = os.path.join(REPO_ROOT, "thirdparty", "discord_social_sdk")
ADDON_BIN = os.path.join(
    REPO_ROOT, "demo", "addons", "discord_social_sdk", "bin"
)

# (link library basename, runtime library basename) per platform.
RUNTIME_LIBS = {
    "Windows": "discord_partner_sdk.dll",
    "Darwin": "libdiscord_partner_sdk.dylib",
    "Linux": "libdiscord_partner_sdk.so",
}


def _copy_tree(src, dst):
    if not os.path.isdir(src):
        sys.exit(f"error: expected directory not found: {src}")
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    print(f"  copied {src} -> {dst}")


def install(sdk_path):
    sdk_path = os.path.abspath(sdk_path)
    print(f"Installing Discord Social SDK from {sdk_path}")
    for sub in ("include", "lib", "bin"):
        _copy_tree(os.path.join(sdk_path, sub), os.path.join(THIRDPARTY, sub))

    runtime = RUNTIME_LIBS[platform.system()]
    # The runtime lib lives under bin/release (Windows .dll) or lib/release
    # (macOS .dylib / Linux .so, which serve as both link and runtime libs).
    src_runtime = _find(os.path.join(THIRDPARTY, "bin"), runtime) or _find(
        os.path.join(THIRDPARTY, "lib"), runtime
    )
    if src_runtime:
        os.makedirs(ADDON_BIN, exist_ok=True)
        dst_runtime = os.path.join(ADDON_BIN, runtime)
        shutil.copy2(src_runtime, dst_runtime)
        print(f"  staged runtime lib -> {dst_runtime}")
        _clear_quarantine(dst_runtime)
    else:
        print(f"  warning: runtime library {runtime} not found under bin/")

    # Windows also ships an arm64 runtime DLL; stage it under bin/arm64/ to
    # match the windows.*.arm64 entries in the .gdextension.
    if platform.system() == "Windows":
        arm_dll = os.path.join(THIRDPARTY, "bin", "release", "arm64", runtime)
        if os.path.isfile(arm_dll):
            arm_dst = os.path.join(ADDON_BIN, "arm64")
            os.makedirs(arm_dst, exist_ok=True)
            shutil.copy2(arm_dll, os.path.join(arm_dst, runtime))
            print(f"  staged arm64 runtime lib -> {os.path.join(arm_dst, runtime)}")

    _clear_quarantine(THIRDPARTY)
    print("Done.")


def _clear_quarantine(path):
    """On macOS, strip the com.apple.quarantine attribute so Gatekeeper does
    not block loading the unsigned SDK dylib during local development."""
    if platform.system() != "Darwin":
        return
    subprocess.run(
        ["xattr", "-r", "-d", "com.apple.quarantine", path],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def merge_universal(arm_path, x86_path):
    """Combine the arm64 and x86_64 macOS dylibs into one universal binary."""
    if platform.system() != "Darwin":
        sys.exit("error: --merge-universal is only supported on macOS")
    # Lay down the arm64 SDK first (headers + lib layout), then lipo the dylib.
    install(arm_path)
    runtime = RUNTIME_LIBS["Darwin"]
    arm_lib = _find(os.path.join(os.path.abspath(arm_path), "bin"), runtime)
    x86_lib = _find(os.path.join(os.path.abspath(x86_path), "bin"), runtime)
    out = os.path.join(ADDON_BIN, runtime)
    subprocess.check_call(["lipo", "-create", arm_lib, x86_lib, "-output", out])
    # Also make the link-time lib universal.
    arm_link = _find(os.path.join(THIRDPARTY, "lib"), runtime)
    x86_link = _find(os.path.join(os.path.abspath(x86_path), "lib"), runtime)
    if arm_link and x86_link:
        subprocess.check_call(
            ["lipo", "-create", arm_link, x86_link, "-output", arm_link]
        )
    print(f"Created universal {runtime} via lipo.")


def _find(root, name):
    for dirpath, _dirs, files in os.walk(root):
        if name in files:
            return os.path.join(dirpath, name)
    return None


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("sdk_path", nargs="?", help="extracted discord_social_sdk folder")
    p.add_argument(
        "--merge-universal",
        nargs=2,
        metavar=("ARM64", "X86_64"),
        help="combine two macOS SDK folders into a universal dylib",
    )
    args = p.parse_args()
    if args.merge_universal:
        merge_universal(*args.merge_universal)
    elif args.sdk_path:
        install(args.sdk_path)
    else:
        p.error("provide an SDK path or --merge-universal ARM64 X86_64")


if __name__ == "__main__":
    main()
