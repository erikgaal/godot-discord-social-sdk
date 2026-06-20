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

### Backend login

To authenticate players against your own backend, let the backend own the token
exchange instead of doing it on the client. Call `request_authorization_code()`,
forward the result to your server, and connect with the token it returns:

```gdscript
func _ready() -> void:
    Discord.authorization_code_received.connect(_on_code)
    Discord.initialize(YOUR_APPLICATION_ID)
    Discord.request_authorization_code()           # PKCE authorize only

func _on_code(success: bool, code: String, redirect_uri: String,
        code_verifier: String, error: String) -> void:
    if not success:
        push_error(error); return
    # POST {code, redirect_uri, code_verifier} to your backend. The backend
    # exchanges them at https://discord.com/api/v10/oauth2/token with
    # grant_type=authorization_code + your client_id/client_secret, then returns
    # the access token (and mints its own session for the player).
    var token := await _exchange_on_backend(code, redirect_uri, code_verifier)
    Discord.connect_with_token(token)
```

Because the backend performs the exchange with its `client_secret`, it *knows*
the token was issued for your app — no client-side token to trust.

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
- `request_authorization_code()` — run only the PKCE authorization step (no token
  exchange) and emit `authorization_code_received` with the `code`,
  `redirect_uri`, and `code_verifier`. For backend-driven login: see *Backend
  login* below.
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
- `authorization_code_received(success: bool, code: String, redirect_uri: String, code_verifier: String, error: String)`
- `rich_presence_updated(success: bool, error: String)`
- `friend_request_sent(success: bool, error: String)`

**Enums** — `STATUS_DISCONNECTED/CONNECTING/CONNECTED/READY/RECONNECTING/DISCONNECTING/HTTP_WAIT`,
`ACTIVITY_PLAYING/STREAMING/LISTENING/WATCHING/CUSTOM/COMPETING`.
