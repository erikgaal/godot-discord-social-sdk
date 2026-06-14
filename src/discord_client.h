#ifndef DISCORD_CLIENT_H
#define DISCORD_CLIENT_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>
#include <memory>

// Forward-declare the SDK client so this header does not pull in discordpp.h
// (which is large and C++20-only). The full type is only needed in the .cpp.
namespace discordpp {
class Client;
}

namespace godot {

// DiscordClient is a Node so it can live in the scene tree and pump the SDK's
// callback queue from _process(). Add one instance to your scene (or use the
// `Discord` autoload provided by the addon) and drive it from GDScript.
class DiscordClient : public Node {
	GDCLASS(DiscordClient, Node)

public:
	// Mirrors discordpp::Client::Status. Kept in sync manually and exposed to
	// GDScript as an enum so scripts can compare against named constants.
	enum Status {
		STATUS_DISCONNECTED = 0,
		STATUS_CONNECTING = 1,
		STATUS_CONNECTED = 2,
		STATUS_READY = 3,
		STATUS_RECONNECTING = 4,
		STATUS_DISCONNECTING = 5,
		STATUS_HTTP_WAIT = 6,
	};

	// Mirrors discordpp::ActivityTypes for the most common presence types.
	enum ActivityType {
		ACTIVITY_PLAYING = 0,
		ACTIVITY_STREAMING = 1,
		ACTIVITY_LISTENING = 2,
		ACTIVITY_WATCHING = 3,
		ACTIVITY_CUSTOM = 4,
		ACTIVITY_COMPETING = 5,
	};

	DiscordClient();
	~DiscordClient();

	// Create the underlying SDK client and wire up log/status callbacks.
	// Must be called before any other method. `application_id` is your
	// Discord application's ID from the Developer Portal.
	void initialize(uint64_t application_id);
	bool is_initialized() const;

	// Authenticate using an access token you already obtained (e.g. via your
	// own backend). Calls UpdateToken then Connect.
	void connect_with_token(const String &access_token);

	// Run the full OAuth2 + PKCE authorization flow. If the Discord overlay is
	// enabled the prompt appears in-game; otherwise it falls back to the
	// browser. On success it connects automatically and emits
	// `authorization_completed`.
	void begin_authorization();

	// Run only the PKCE authorization step (no token exchange) and emit
	// `authorization_code_received` with the authorization `code`, the
	// `redirect_uri`, and the PKCE `code_verifier`. Forward all three to your own
	// backend so it can exchange the code for a token using its client_secret
	// (POST https://discord.com/api/v10/oauth2/token), then hand the resulting
	// access token back to the client to pass to connect_with_token(). Use this
	// instead of begin_authorization() when your backend should own the token.
	void request_authorization_code();

	// Tell the SDK which window/process the authentication overlay should
	// attach to. initialize() already sets this to the running Godot process;
	// override it only if your game window lives in a different process.
	void set_game_window_pid(int32_t pid);

	// Tear down the connection. Safe to call when not connected.
	void disconnect_client();

	// Rich presence shortcut for the common case — sets only details/state/type.
	// Equivalent to set_activity({"details": ..., "state": ..., "type": ...}).
	void set_rich_presence(const String &details, const String &state, ActivityType activity_type);

	// Full Rich Presence (Activity). Pass a Dictionary; recognised keys:
	//   type:int            — an ActivityType value (default ACTIVITY_PLAYING)
	//   name:String         — app/game name override (usually left to the SDK)
	//   details:String      — main line ("Playing Capture the Flag")
	//   state:String        — secondary line ("In Match")
	//   timestamps:{ start:int, end:int }   — Unix seconds; Discord shows elapsed/remaining
	//   party:{ id:String, size:int, max:int }   — shows "size of max"
	//   assets:{ large_image:String, large_text:String, small_image:String, small_text:String }
	//                         — image values are Art Asset names from the Dev Portal (or URLs)
	// Omitted keys are left unset; empty strings are ignored.
	void set_activity(const Dictionary &activity);

	void clear_rich_presence();

	// Send a Discord friend request to the given username.
	void send_discord_friend_request(const String &username);

	// Returns { id, username, display_name } for the connected user, or an
	// empty Dictionary if not ready.
	Dictionary get_current_user() const;

	Status get_status() const;

	// Returns the Discord Social SDK version the binding is running against:
	// { major, minor, patch, hash, string }. Static SDK info — works without
	// initialize().
	Dictionary get_sdk_version() const;

	// Pumps discordpp::RunCallbacks() every frame. Public so it can be driven
	// manually in tests, but normally called automatically from _process().
	void poll();

	// Godot virtuals — must be public for godot-cpp's virtual registration.
	void _process(double delta) override;
	void _notification(int what);

protected:
	static void _bind_methods();

private:
	std::shared_ptr<discordpp::Client> client_;
	uint64_t application_id_ = 0;
	Status status_ = STATUS_DISCONNECTED;

	void _on_status_changed(int32_t status, int32_t error, int32_t error_detail);
};

} // namespace godot

VARIANT_ENUM_CAST(godot::DiscordClient::Status);
VARIANT_ENUM_CAST(godot::DiscordClient::ActivityType);

#endif // DISCORD_CLIENT_H
