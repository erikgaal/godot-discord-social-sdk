# Discord Social SDK goes here

The Discord Social SDK binaries are **not** redistributable through this
repository — they are gated behind the Discord Developer Portal and tied to
your application. You must download them yourself.

## How to obtain the SDK

1. Open the [Discord Developer Portal](https://discord.com/developers/applications)
   and select (or create) your application.
2. Go to the **Discord Social SDK → Downloads** tab.
3. Download the latest **C++** package for each platform you target
   (Windows, macOS, Linux).
4. Extract each archive. Every archive contains a `discord_social_sdk/`
   folder with this layout:

   ```
   discord_social_sdk/
   ├── include/
   │   ├── discordpp.h          # main header (single-header style)
   │   └── ...
   ├── lib/release/             # link-time libraries
   │   ├── discord_partner_sdk.lib          (Windows)
   │   ├── libdiscord_partner_sdk.dylib     (macOS)
   │   └── libdiscord_partner_sdk.so        (Linux)
   └── bin/release/             # runtime libraries shipped with your game
       ├── discord_partner_sdk.dll          (Windows)
       ├── libdiscord_partner_sdk.dylib     (macOS)
       └── libdiscord_partner_sdk.so        (Linux)
   ```

## Installing into this repo

Use the helper script from the repo root, pointing it at the extracted folder
for the current platform:

```bash
python scripts/setup_sdk.py /path/to/extracted/discord_social_sdk
```

It copies `include/`, `lib/`, and `bin/` into
`thirdparty/discord_social_sdk/` (which this README lives in) and copies the
matching runtime library into `demo/addons/discord_social_sdk/bin/` so the
addon can load it at runtime.

On macOS, run the script once for the arm64 package and once for the x86_64
package (or pass both with `--merge-universal`) to produce a universal
`libdiscord_partner_sdk.dylib` via `lipo`.

After this, the directory should contain `include/`, `lib/`, and `bin/`.
Those subfolders are git-ignored on purpose.
