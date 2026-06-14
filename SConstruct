#!/usr/bin/env python
"""SConstruct for the Godot Discord Social SDK GDExtension.

Builds the C++ binding on top of godot-cpp and links it against the Discord
Social SDK (downloaded separately into thirdparty/discord_social_sdk — see
that folder's README). Output binaries are written directly into the addon's
bin/ directory so the demo project can load them.

Examples:
    scons target=template_debug
    scons target=template_release
    scons platform=macos arch=universal target=template_release
"""
import os

# Pull in the godot-cpp build environment (provides platform/arch/target
# options and the godot-cpp static library as a dependency).
env = SConscript("godot-cpp/SConstruct")

platform = env["platform"]
arch = env["arch"]

# --- Discord Social SDK paths --------------------------------------------
SDK_DIR = os.path.join("thirdparty", "discord_social_sdk")
SDK_INCLUDE = os.path.join(SDK_DIR, "include")
SDK_LIB = os.path.join(SDK_DIR, "lib", "release")

# Windows ships a separate arm64 import library under lib/release/arm64/.
if platform == "windows" and arch == "arm64":
    SDK_LIB = os.path.join(SDK_LIB, "arm64")

if not os.path.isdir(SDK_INCLUDE):
    print(
        "\n[!] Discord Social SDK not found at %s\n"
        "    Download it from the Discord Developer Portal and install with:\n"
        "        python scripts/setup_sdk.py /path/to/discord_social_sdk\n"
        % SDK_INCLUDE
    )
    Exit(1)

# C++20 is required by the Discord Social SDK.
if env.get("is_msvc", False):
    env.Append(CXXFLAGS=["/std:c++20"])
else:
    env.Append(CXXFLAGS=["-std=c++20"])

env.Append(CPPPATH=["src/", SDK_INCLUDE])
env.Append(LIBPATH=[SDK_LIB])

# Link against the Discord partner SDK. SCons resolves "discord_partner_sdk"
# to discord_partner_sdk.lib (Windows) or libdiscord_partner_sdk.{dylib,so}.
env.Append(LIBS=["discord_partner_sdk"])

# On macOS/Linux the runtime dylib/so ships next to the extension binary, so
# tell the loader to look beside the loaded module.
if platform == "macos":
    env.Append(LINKFLAGS=["-Wl,-rpath,@loader_path"])
elif platform == "linux":
    env.Append(LINKFLAGS=["-Wl,-rpath,'$ORIGIN'"])

sources = Glob("src/*.cpp")

# --- Output path ---------------------------------------------------------
bin_dir = os.path.join("demo", "addons", "discord_social_sdk", "bin")
libname = "libdiscord_social_sdk{}{}".format(env["suffix"], env["SHLIBSUFFIX"])

library = env.SharedLibrary(os.path.join(bin_dir, libname), source=sources)
Default(library)
