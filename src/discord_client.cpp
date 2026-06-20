#include "discord_client.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// discordpp.h is included WITHOUT DISCORDPP_IMPLEMENTATION here; the
// implementation lives in discord_sdk_impl.cpp.
#include <discordpp.h>

#include <optional>
#include <string>

using namespace godot;

namespace {
// Helpers to convert between Godot String (UTF-8) and std::string.
std::string to_std(const String &s) {
	return std::string(s.utf8().get_data());
}
String to_godot(const std::string &s) {
	return String::utf8(s.c_str());
}

// Set a string field only when present and non-empty (empty == "leave unset").
template <typename Setter>
void apply_string(const Dictionary &d, const char *key, Setter setter) {
	if (d.has(key)) {
		String s = d[key];
		if (!s.is_empty()) {
			setter(to_std(s));
		}
	}
}

// Translate a GDScript Dictionary into a discordpp::Activity. Unknown keys are ignored.
// Kept here (not in the header) so discordpp.h stays out of the public header.
discordpp::Activity build_activity(const Dictionary &d) {
	discordpp::Activity activity;

	int type_val = 0;  // 0 == ACTIVITY_PLAYING
	if (d.has("type")) {
		type_val = (int)(int64_t)d["type"];
	}
	activity.SetType((discordpp::ActivityTypes)type_val);

	apply_string(d, "name", [&](std::string v) { activity.SetName(v); });
	apply_string(d, "details", [&](std::string v) { activity.SetDetails(v); });
	apply_string(d, "details_url", [&](std::string v) { activity.SetDetailsUrl(v); });
	apply_string(d, "state", [&](std::string v) { activity.SetState(v); });
	apply_string(d, "state_url", [&](std::string v) { activity.SetStateUrl(v); });

	if (d.has("status_display_type")) {
		activity.SetStatusDisplayType((discordpp::StatusDisplayTypes)(int)(int64_t)d["status_display_type"]);
	}

	if (d.has("timestamps")) {
		Dictionary t = d["timestamps"];
		discordpp::ActivityTimestamps ts;
		if (t.has("start")) {
			ts.SetStart((uint64_t)(int64_t)t["start"]);
		}
		if (t.has("end")) {
			ts.SetEnd((uint64_t)(int64_t)t["end"]);
		}
		activity.SetTimestamps(ts);
	}

	if (d.has("party")) {
		Dictionary p = d["party"];
		discordpp::ActivityParty party;
		apply_string(p, "id", [&](std::string v) { party.SetId(v); });
		if (p.has("size")) {
			party.SetCurrentSize((int32_t)(int64_t)p["size"]);
		}
		if (p.has("max")) {
			party.SetMaxSize((int32_t)(int64_t)p["max"]);
		}
		if (p.has("privacy")) {
			party.SetPrivacy((discordpp::ActivityPartyPrivacy)(int)(int64_t)p["privacy"]);
		}
		activity.SetParty(party);
	}

	if (d.has("assets")) {
		Dictionary a = d["assets"];
		discordpp::ActivityAssets assets;
		apply_string(a, "large_image", [&](std::string v) { assets.SetLargeImage(v); });
		apply_string(a, "large_text", [&](std::string v) { assets.SetLargeText(v); });
		apply_string(a, "large_url", [&](std::string v) { assets.SetLargeUrl(v); });
		apply_string(a, "small_image", [&](std::string v) { assets.SetSmallImage(v); });
		apply_string(a, "small_text", [&](std::string v) { assets.SetSmallText(v); });
		apply_string(a, "small_url", [&](std::string v) { assets.SetSmallUrl(v); });
		apply_string(a, "invite_cover_image", [&](std::string v) { assets.SetInviteCoverImage(v); });
		activity.SetAssets(assets);
	}

	// Up to two link buttons. Each entry is { label, url }; both are required for
	// the button to be useful, so skip entries missing either.
	if (d.has("buttons")) {
		Array buttons = d["buttons"];
		for (int i = 0; i < buttons.size(); i++) {
			Dictionary b = buttons[i];
			String label = b.get("label", "");
			String url = b.get("url", "");
			if (label.is_empty() || url.is_empty()) {
				continue;
			}
			discordpp::ActivityButton button;
			button.SetLabel(to_std(label));
			button.SetUrl(to_std(url));
			activity.AddButton(button);
		}
	}

	return activity;
}
} // namespace

DiscordClient::DiscordClient() {}

DiscordClient::~DiscordClient() {
	// The shared_ptr releases the SDK client; discordpp::Client's destructor
	// tears down the connection cleanly.
}

void DiscordClient::_bind_methods() {
	ClassDB::bind_method(D_METHOD("initialize", "application_id"), &DiscordClient::initialize);
	ClassDB::bind_method(D_METHOD("is_initialized"), &DiscordClient::is_initialized);
	ClassDB::bind_method(D_METHOD("connect_with_token", "access_token"), &DiscordClient::connect_with_token);
	ClassDB::bind_method(D_METHOD("begin_authorization"), &DiscordClient::begin_authorization);
	ClassDB::bind_method(D_METHOD("request_authorization_code"), &DiscordClient::request_authorization_code);
	ClassDB::bind_method(D_METHOD("set_game_window_pid", "pid"), &DiscordClient::set_game_window_pid);
	ClassDB::bind_method(D_METHOD("disconnect_client"), &DiscordClient::disconnect_client);
	ClassDB::bind_method(D_METHOD("set_rich_presence", "details", "state", "activity_type"), &DiscordClient::set_rich_presence, DEFVAL(ACTIVITY_PLAYING));
	ClassDB::bind_method(D_METHOD("set_activity", "activity"), &DiscordClient::set_activity);
	ClassDB::bind_method(D_METHOD("clear_rich_presence"), &DiscordClient::clear_rich_presence);
	ClassDB::bind_method(D_METHOD("send_discord_friend_request", "username"), &DiscordClient::send_discord_friend_request);
	ClassDB::bind_method(D_METHOD("get_current_user"), &DiscordClient::get_current_user);
	ClassDB::bind_method(D_METHOD("get_status"), &DiscordClient::get_status);
	ClassDB::bind_method(D_METHOD("get_sdk_version"), &DiscordClient::get_sdk_version);
	ClassDB::bind_method(D_METHOD("poll"), &DiscordClient::poll);

	// Status enum
	BIND_ENUM_CONSTANT(STATUS_DISCONNECTED);
	BIND_ENUM_CONSTANT(STATUS_CONNECTING);
	BIND_ENUM_CONSTANT(STATUS_CONNECTED);
	BIND_ENUM_CONSTANT(STATUS_READY);
	BIND_ENUM_CONSTANT(STATUS_RECONNECTING);
	BIND_ENUM_CONSTANT(STATUS_DISCONNECTING);
	BIND_ENUM_CONSTANT(STATUS_HTTP_WAIT);

	// ActivityType enum
	BIND_ENUM_CONSTANT(ACTIVITY_PLAYING);
	BIND_ENUM_CONSTANT(ACTIVITY_STREAMING);
	BIND_ENUM_CONSTANT(ACTIVITY_LISTENING);
	BIND_ENUM_CONSTANT(ACTIVITY_WATCHING);
	BIND_ENUM_CONSTANT(ACTIVITY_CUSTOM);
	BIND_ENUM_CONSTANT(ACTIVITY_COMPETING);

	// StatusDisplayType enum
	BIND_ENUM_CONSTANT(STATUS_DISPLAY_NAME);
	BIND_ENUM_CONSTANT(STATUS_DISPLAY_STATE);
	BIND_ENUM_CONSTANT(STATUS_DISPLAY_DETAILS);

	// PartyPrivacy enum
	BIND_ENUM_CONSTANT(PARTY_PRIVACY_PRIVATE);
	BIND_ENUM_CONSTANT(PARTY_PRIVACY_PUBLIC);

	// Signals. The `status` argument is typed as the DiscordClient.Status enum
	// so GDScript handlers get the enum type (not a bare int).
	ADD_SIGNAL(MethodInfo("status_changed",
			PropertyInfo(Variant::INT, "status", PROPERTY_HINT_ENUM,
					"Disconnected:0,Connecting:1,Connected:2,Ready:3,Reconnecting:4,Disconnecting:5,HttpWait:6",
					PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "DiscordClient.Status"),
			PropertyInfo(Variant::INT, "error"),
			PropertyInfo(Variant::INT, "error_detail")));
	ADD_SIGNAL(MethodInfo("client_ready"));
	ADD_SIGNAL(MethodInfo("log_message",
			PropertyInfo(Variant::STRING, "message"),
			PropertyInfo(Variant::INT, "severity")));
	ADD_SIGNAL(MethodInfo("authorization_completed",
			PropertyInfo(Variant::BOOL, "success"),
			PropertyInfo(Variant::STRING, "error")));
	ADD_SIGNAL(MethodInfo("authorization_code_received",
			PropertyInfo(Variant::BOOL, "success"),
			PropertyInfo(Variant::STRING, "code"),
			PropertyInfo(Variant::STRING, "redirect_uri"),
			PropertyInfo(Variant::STRING, "code_verifier"),
			PropertyInfo(Variant::STRING, "error")));
	ADD_SIGNAL(MethodInfo("rich_presence_updated",
			PropertyInfo(Variant::BOOL, "success"),
			PropertyInfo(Variant::STRING, "error")));
	ADD_SIGNAL(MethodInfo("friend_request_sent",
			PropertyInfo(Variant::BOOL, "success"),
			PropertyInfo(Variant::STRING, "error")));
}

void DiscordClient::initialize(uint64_t application_id) {
	if (client_) {
		WARN_PRINT("DiscordClient already initialized.");
		return;
	}
	application_id_ = application_id;
	client_ = std::make_shared<discordpp::Client>();
	client_->SetApplicationId(application_id);

	// Attach the auth overlay to the running Godot process by default so the
	// authentication prompt can render in-game instead of opening a browser.
	client_->SetGameWindowPid((int32_t)OS::get_singleton()->get_process_id());

	client_->AddLogCallback(
			[this](std::string message, discordpp::LoggingSeverity severity) {
				call_deferred("emit_signal", "log_message", to_godot(message), (int)severity);
			},
			discordpp::LoggingSeverity::Info);

	client_->SetStatusChangedCallback(
			[this](discordpp::Client::Status status, discordpp::Client::Error error, int32_t error_detail) {
				_on_status_changed((int32_t)status, (int32_t)error, error_detail);
			});
}

bool DiscordClient::is_initialized() const {
	return client_ != nullptr;
}

void DiscordClient::_on_status_changed(int32_t status, int32_t error, int32_t error_detail) {
	status_ = (Status)status;
	emit_signal("status_changed", status, error, error_detail);
	if (status_ == STATUS_READY) {
		emit_signal("client_ready");
	}
}

void DiscordClient::connect_with_token(const String &access_token) {
	ERR_FAIL_NULL_MSG(client_, "DiscordClient not initialized. Call initialize() first.");
	client_->UpdateToken(discordpp::AuthorizationTokenType::Bearer, to_std(access_token),
			[this](discordpp::ClientResult result) {
				if (result.Successful()) {
					client_->Connect();
				} else {
					emit_signal("authorization_completed", false, to_godot(result.Error()));
				}
			});
}

void DiscordClient::begin_authorization() {
	ERR_FAIL_NULL_MSG(client_, "DiscordClient not initialized. Call initialize() first.");

	// PKCE: create a code verifier/challenge pair. The verifier must survive
	// until GetToken, so capture it in the lambda chain.
	auto verifier = std::make_shared<discordpp::AuthorizationCodeVerifier>(
			client_->CreateAuthorizationCodeVerifier());

	discordpp::AuthorizationArgs args{};
	args.SetClientId(application_id_);
	args.SetScopes(discordpp::Client::GetDefaultPresenceScopes());
	args.SetCodeChallenge(verifier->Challenge());

	client_->Authorize(args,
			[this, verifier](discordpp::ClientResult result, std::string code, std::string redirect_uri) {
				if (!result.Successful()) {
					emit_signal("authorization_completed", false, to_godot(result.Error()));
					return;
				}
				client_->GetToken(application_id_, code, verifier->Verifier(), redirect_uri,
						[this](discordpp::ClientResult token_result, std::string access_token,
								std::string /*refresh_token*/, discordpp::AuthorizationTokenType /*type*/,
								int32_t /*expires_in*/, std::string /*scopes*/) {
							if (!token_result.Successful()) {
								emit_signal("authorization_completed", false, to_godot(token_result.Error()));
								return;
							}
							client_->UpdateToken(discordpp::AuthorizationTokenType::Bearer, access_token,
									[this](discordpp::ClientResult update_result) {
										if (update_result.Successful()) {
											client_->Connect();
											emit_signal("authorization_completed", true, String());
										} else {
											emit_signal("authorization_completed", false, to_godot(update_result.Error()));
										}
									});
						});
			});
}

void DiscordClient::request_authorization_code() {
	ERR_FAIL_NULL_MSG(client_, "DiscordClient not initialized. Call initialize() first.");

	// PKCE: same verifier/challenge as begin_authorization(), but we stop after
	// Authorize() and hand the code + verifier to GDScript so the game's backend
	// can perform the token exchange with its client_secret.
	auto verifier = std::make_shared<discordpp::AuthorizationCodeVerifier>(
			client_->CreateAuthorizationCodeVerifier());

	discordpp::AuthorizationArgs args{};
	args.SetClientId(application_id_);
	args.SetScopes(discordpp::Client::GetDefaultPresenceScopes());
	args.SetCodeChallenge(verifier->Challenge());

	client_->Authorize(args,
			[this, verifier](discordpp::ClientResult result, std::string code, std::string redirect_uri) {
				if (!result.Successful()) {
					emit_signal("authorization_code_received", false, String(), String(), String(),
							to_godot(result.Error()));
					return;
				}
				emit_signal("authorization_code_received", true, to_godot(code), to_godot(redirect_uri),
						to_godot(verifier->Verifier()), String());
			});
}

void DiscordClient::set_game_window_pid(int32_t pid) {
	ERR_FAIL_NULL_MSG(client_, "DiscordClient not initialized. Call initialize() first.");
	client_->SetGameWindowPid(pid);
}

void DiscordClient::disconnect_client() {
	if (client_) {
		client_->Disconnect();
	}
}

void DiscordClient::set_rich_presence(const String &details, const String &state, ActivityType activity_type) {
	Dictionary d;
	d["type"] = (int)activity_type;
	if (!details.is_empty()) {
		d["details"] = details;
	}
	if (!state.is_empty()) {
		d["state"] = state;
	}
	set_activity(d);
}

void DiscordClient::set_activity(const Dictionary &activity) {
	ERR_FAIL_NULL_MSG(client_, "DiscordClient not initialized. Call initialize() first.");
	client_->UpdateRichPresence(build_activity(activity), [this](discordpp::ClientResult result) {
		emit_signal("rich_presence_updated", result.Successful(), to_godot(result.Error()));
	});
}

void DiscordClient::clear_rich_presence() {
	ERR_FAIL_NULL_MSG(client_, "DiscordClient not initialized. Call initialize() first.");
	// An empty/default Activity clears the presence.
	discordpp::Activity activity;
	client_->UpdateRichPresence(activity, [this](discordpp::ClientResult result) {
		emit_signal("rich_presence_updated", result.Successful(), to_godot(result.Error()));
	});
}

void DiscordClient::send_discord_friend_request(const String &username) {
	ERR_FAIL_NULL_MSG(client_, "DiscordClient not initialized. Call initialize() first.");
	client_->SendDiscordFriendRequest(to_std(username), [this](discordpp::ClientResult result) {
		emit_signal("friend_request_sent", result.Successful(), to_godot(result.Error()));
	});
}

Dictionary DiscordClient::get_current_user() const {
	Dictionary out;
	if (!client_) {
		return out;
	}
	std::optional<discordpp::UserHandle> user = client_->GetCurrentUserV2();
	if (!user.has_value()) {
		return out;
	}
	out["id"] = String::num_uint64(user->Id());
	out["username"] = to_godot(user->Username());
	out["display_name"] = to_godot(user->DisplayName());
	return out;
}

DiscordClient::Status DiscordClient::get_status() const {
	return status_;
}

Dictionary DiscordClient::get_sdk_version() const {
	Dictionary out;
	int major = discordpp::Client::GetVersionMajor();
	int minor = discordpp::Client::GetVersionMinor();
	int patch = discordpp::Client::GetVersionPatch();
	out["major"] = major;
	out["minor"] = minor;
	out["patch"] = patch;
	out["hash"] = to_godot(discordpp::Client::GetVersionHash());
	out["string"] = vformat("%d.%d.%d", major, minor, patch);
	return out;
}

void DiscordClient::poll() {
	discordpp::RunCallbacks();
}

void DiscordClient::_process(double /*delta*/) {
	// Pump the SDK callback queue every frame on the main thread, so signals
	// emitted from callbacks are delivered safely. Skip in the editor.
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	discordpp::RunCallbacks();
}

void DiscordClient::_notification(int what) {
	if (what == NOTIFICATION_PREDELETE) {
		disconnect_client();
	}
}
