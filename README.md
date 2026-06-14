# Godot Discord Social SDK

[![CI](https://github.com/erikgaal/godot-discord-social-sdk/actions/workflows/ci.yml/badge.svg)](https://github.com/erikgaal/godot-discord-social-sdk/actions/workflows/ci.yml)

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

- **`ci.yml`** — builds debug+release for all four platform/arch targets (via
  the reusable `_build.yml`) and runs the headless tests on Linux.
- **`release.yml`** — manual dispatch: builds every platform, packages the
  addon, tags the commit, and attaches `discord_social_sdk-<version>.zip` to a
  **draft** GitHub Release. Release archives contain only this project's
  binaries — **not** Discord's runtime libraries (users add those from their own
  Developer Portal download).
- **`_build.yml`** — reusable build job shared by the two above.

### Providing the SDK to CI
The SDK can't be fetched anonymously, so each native build needs a download
URL. The Developer Portal generates a **fresh, ~1-hour signed URL** per
download, so the workflows take it as a dispatch input rather than storing it:

1. Download the SDK link from the Developer Portal (right-click → copy link, or
   start a download and copy its URL).
2. **Actions → CI → Run workflow**, paste the URL into `sdk_url`, and run. The
   job downloads + builds within the URL's validity window.

Push/PR runs (which have no URL input) skip the native steps with a warning —
expected, and the only option for fork PRs, which can't read secrets anyway. If
you have a *stable* SDK URL, you can instead set it as a repository secret
**`DISCORD_SDK_URL`** and the workflows will use it automatically without a
dispatch input.

### Cutting a release
**Actions → Release → Run workflow**, enter the `version` (e.g. `0.1.0`) and a
fresh `sdk_url`. It builds all platforms, creates tag `v<version>` on the
current commit, and uploads a draft release zip for you to review and publish.

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
