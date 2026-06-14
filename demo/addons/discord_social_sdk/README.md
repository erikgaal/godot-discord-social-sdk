# Discord Social SDK — Godot addon

A Godot 4 GDExtension binding for the [Discord Social SDK](https://discord.com/developers/docs/discord-social-sdk/overview).

## Install

1. Copy this `discord_social_sdk` folder into your project's `res://addons/`.
2. Download the Discord Social SDK runtime library for each platform you ship
   (from the Discord Developer Portal → your app → **Discord Social SDK →
   Downloads**) and place the runtime libs in `bin/`:
   - macOS: `libdiscord_partner_sdk.dylib` (universal)
   - Linux: `libdiscord_partner_sdk.so`
   - Windows x64: `discord_partner_sdk.dll`
   - Windows arm64: `arm64/discord_partner_sdk.dll`
   The compiled extension binaries (`libdiscord_social_sdk.*`) go in `bin/` too
   — grab them from a release or build them (see the repo root README).
3. Enable **Discord Social SDK** in `Project → Project Settings → Plugins`.
   This registers a `Discord` autoload singleton.

## Usage

```gdscript
func _ready() -> void:
    Discord.status_changed.connect(_on_status_changed)
    Discord.client_ready.connect(_on_ready)
    Discord.initialize(YOUR_APPLICATION_ID)        # int from the Dev Portal
    Discord.begin_authorization()                  # opens Discord in browser

func _on_ready() -> void:
    var user := Discord.get_current_user()
    print("Connected as %s" % user.get("username", "?"))
    Discord.set_rich_presence("In the main menu", "Playing solo",
        DiscordClient.ACTIVITY_PLAYING)

func _on_status_changed(status: int, error: int, error_detail: int) -> void:
    print("Status: ", status)
```

If you already have an OAuth2 bearer token (e.g. from your own backend), skip
`begin_authorization()` and call `Discord.connect_with_token(token)` instead.

## API

`DiscordClient` is a `Node`. It pumps the SDK callback queue every frame from
`_process`, so it must be in the scene tree (the `Discord` autoload is).

**Methods**
- `initialize(application_id: int)` — create the client; call first.
- `begin_authorization()` — full OAuth2 + PKCE flow (opens browser, connects).
- `connect_with_token(access_token: String)` — connect with an existing token.
- `set_game_window_pid(pid: int)` — window the auth overlay attaches to;
  `initialize()` already sets this to the Godot process.
- `disconnect_client()`
- `set_rich_presence(details: String, state: String, activity_type := DiscordClient.ACTIVITY_PLAYING)`
- `clear_rich_presence()`
- `send_discord_friend_request(username: String)`
- `get_current_user() -> Dictionary` — `{ id, username, display_name }`.
- `get_status() -> int` — one of the `STATUS_*` constants.
- `get_sdk_version() -> Dictionary` — `{ major, minor, patch, hash, string }`
  for the Discord Social SDK the addon is running against. Works without
  `initialize()`.

## Versioning

This addon uses its own SemVer, independent of the Discord Social SDK version.
Each release notes the SDK version it was built against; call
`get_sdk_version()` to see what's actually installed at runtime.

**Signals**
- `status_changed(status: int, error: int, error_detail: int)`
- `client_ready()` — fired when status becomes `STATUS_READY`.
- `log_message(message: String, severity: int)`
- `authorization_completed(success: bool, error: String)`
- `rich_presence_updated(success: bool, error: String)`
- `friend_request_sent(success: bool, error: String)`

**Enums** — `STATUS_DISCONNECTED/CONNECTING/CONNECTED/READY/RECONNECTING/DISCONNECTING/HTTP_WAIT`,
`ACTIVITY_PLAYING/STREAMING/LISTENING/WATCHING/CUSTOM/COMPETING`.
