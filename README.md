# Godot Discord Social SDK

<!-- Replace OWNER/REPO once published, then this badge goes live. -->
[![CI](https://github.com/OWNER/REPO/actions/workflows/ci.yml/badge.svg)](https://github.com/OWNER/REPO/actions/workflows/ci.yml)

A **Godot 4 GDExtension** binding for the [Discord Social SDK](https://discord.com/developers/docs/discord-social-sdk/overview)
— the modern replacement for the deprecated Discord Game SDK. Written in C++
on top of [godot-cpp](https://github.com/godotengine/godot-cpp), usable from
GDScript, and cross-platform: **Windows (x64/arm64), macOS (universal arm64 +
x86_64), and Linux (x64)**.

| Feature | Status |
| --- | --- |
| Client lifecycle + status events | ✅ |
| OAuth2 + PKCE authorization flow | ✅ |
| Connect with an existing access token | ✅ |
| Rich presence (activities) | ✅ |
| Friend requests | ✅ |
| Current user info | ✅ |
| SDK logging surfaced as a signal | ✅ |
| Lobbies / voice / messaging | 🚧 not yet wrapped (extension points in place) |

> The Discord Social SDK binaries are gated behind the Discord Developer Portal
> and tied to your application. They are **not** included in this repo — you
> download them yourself (see below). Only the binding code is MIT-licensed.

## Repository layout

```
src/                      C++ GDExtension source (the binding)
godot-cpp/                godot-cpp (git submodule)
thirdparty/
  discord_social_sdk/     where you install the downloaded SDK (git-ignored)
scripts/setup_sdk.py      installs a downloaded SDK into this repo
SConstruct                build script (links the SDK, outputs into the addon)
demo/
  addons/discord_social_sdk/   the distributable addon (this is what you ship)
  main.tscn / main.gd          interactive demo
  tests/run_tests.gd           headless test suite
.github/workflows/build.yml    CI: build all platforms + run tests
```

## Using the addon in your own project

1. Copy `demo/addons/discord_social_sdk/` into your project's `res://addons/`.
2. Add the compiled extension binaries and the Discord runtime libraries to
   `addons/discord_social_sdk/bin/` (from a release, or build them — below).
3. Enable **Discord Social SDK** under `Project → Project Settings → Plugins`.
4. Use the `Discord` autoload singleton. See
   [`demo/addons/discord_social_sdk/README.md`](demo/addons/discord_social_sdk/README.md)
   for the full API and a usage snippet.

## Building from source

### Prerequisites
- Python 3 + [SCons](https://scons.org) (`pip install scons`)
- A C++20 compiler: MSVC (Windows), Clang/Xcode (macOS), GCC/Clang (Linux)
- This repo cloned **with submodules**:
  ```bash
  git clone --recursive <repo-url>
  # or, after a plain clone:
  git submodule update --init --recursive
  ```

### 1. Install the Discord Social SDK
Download the C++ SDK from the Discord Developer Portal (your app → **Discord
Social SDK → Downloads**), extract it, then:

```bash
python scripts/setup_sdk.py /path/to/extracted/discord_social_sdk
```

This copies the headers/libs into `thirdparty/discord_social_sdk/` and stages
the runtime library into the addon's `bin/`. The desktop download already
contains Windows, macOS (universal) and Linux libraries.

### 2. Build
```bash
# Current platform, debug + release:
scons target=template_debug
scons target=template_release

# Explicit platform/arch examples:
scons platform=macos   arch=universal target=template_release
scons platform=linux   arch=x86_64    target=template_release
scons platform=windows arch=x86_64    target=template_release
scons platform=windows arch=arm64     target=template_release
```

Binaries are written to `demo/addons/discord_social_sdk/bin/` with names like
`libdiscord_social_sdk.macos.template_release.universal.dylib`, matching the
entries in `discord_social_sdk.gdextension`.

## Running the demo

Open the `demo/` folder in Godot 4.4+ and run it. The demo lets you enter your
application ID, authorize via browser (or paste a token), set rich presence,
and send friend requests, with a live log of SDK events.

## Tests

A dependency-free headless test suite verifies class registration and the
method/signal/enum surface:

```bash
godot --headless --path demo --import          # first time, registers the extension
godot --headless --path demo --script res://tests/run_tests.gd
```

It exits non-zero on failure and runs in CI.

## CI & releases

Three workflows under `.github/workflows/`:

- **`ci.yml`** — on push/PR: builds debug+release for all four platform/arch
  targets (via the reusable `_build.yml`) and runs the headless tests on Linux.
- **`release.yml`** — on a `v*` tag (or manual dispatch): builds every platform,
  packages the addon, and attaches `discord_social_sdk-<version>.zip` to a
  **draft** GitHub Release. Release archives contain only this project's
  binaries — **not** Discord's runtime libraries (users add those from their own
  Developer Portal download).
- **`_build.yml`** — reusable build job shared by the two above.

### Required secret
Because the SDK can't be fetched anonymously, set a repository secret
**`DISCORD_SDK_URL`** to a direct download URL for a zipped SDK (the archive must
contain a `discord_social_sdk/` folder with `include/discordpp.h`). Host it
somewhere your CI can reach (e.g. a private release asset or object store).
Without the secret the native build/test steps are skipped — expected on fork
PRs, which can't read secrets.

### Cutting a release
```bash
git tag v0.1.0 && git push origin v0.1.0
```
This runs `release.yml` and creates a draft release; review and publish it.

## Troubleshooting

- **macOS: "library load disallowed by system policy" / Gatekeeper** — the
  downloaded SDK dylib is unsigned and quarantined. `setup_sdk.py` strips the
  quarantine attribute automatically; if you copied libs in manually, run:
  ```bash
  xattr -r -d com.apple.quarantine demo/addons/discord_social_sdk/bin
  ```
  For distributed builds you must code-sign and notarize the dylib as part of
  your app's signing.
- **`Could not find base class "DiscordClient"`** — the extension binary for
  your platform isn't present in `bin/`, or you haven't run `--import` once.

## Licensing

The binding code in this repository is MIT-licensed (see `LICENSE`). The
Discord Social SDK itself is Discord's property, governed by the Discord
Developer Terms of Service — obtain and redistribute it per Discord's terms.
