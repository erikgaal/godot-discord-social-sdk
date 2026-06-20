extends Control

## Demo for the Discord Social SDK addon. Drive the `Discord` autoload
## singleton and surface everything in the UI + log panel.

@onready var app_id_edit: LineEdit = %AppIdEdit
@onready var token_edit: LineEdit = %TokenEdit
@onready var status_label: Label = %StatusLabel
@onready var user_label: Label = %UserLabel
@onready var details_edit: LineEdit = %DetailsEdit
@onready var state_edit: LineEdit = %StateEdit
@onready var type_option: OptionButton = %TypeOption
@onready var status_display_option: OptionButton = %StatusDisplayOption
@onready var elapsed_check: CheckBox = %ElapsedCheck
@onready var party_id_edit: LineEdit = %PartyIdEdit
@onready var party_size_spin: SpinBox = %PartySizeSpin
@onready var party_max_spin: SpinBox = %PartyMaxSpin
@onready var party_privacy_option: OptionButton = %PartyPrivacyOption
@onready var large_image_edit: LineEdit = %LargeImageEdit
@onready var large_text_edit: LineEdit = %LargeTextEdit
@onready var small_image_edit: LineEdit = %SmallImageEdit
@onready var small_text_edit: LineEdit = %SmallTextEdit
@onready var button1_label_edit: LineEdit = %Button1LabelEdit
@onready var button1_url_edit: LineEdit = %Button1UrlEdit
@onready var button2_label_edit: LineEdit = %Button2LabelEdit
@onready var button2_url_edit: LineEdit = %Button2UrlEdit
@onready var friend_edit: LineEdit = %FriendEdit

const ACTIVITY_TYPES := [
	["Playing", DiscordClient.ACTIVITY_PLAYING],
	["Streaming", DiscordClient.ACTIVITY_STREAMING],
	["Listening", DiscordClient.ACTIVITY_LISTENING],
	["Watching", DiscordClient.ACTIVITY_WATCHING],
	["Custom", DiscordClient.ACTIVITY_CUSTOM],
	["Competing", DiscordClient.ACTIVITY_COMPETING],
]

const STATUS_DISPLAY_TYPES := [
	["Name", DiscordClient.STATUS_DISPLAY_NAME],
	["State", DiscordClient.STATUS_DISPLAY_STATE],
	["Details", DiscordClient.STATUS_DISPLAY_DETAILS],
]

const PARTY_PRIVACIES := [
	["Private", DiscordClient.PARTY_PRIVACY_PRIVATE],
	["Public", DiscordClient.PARTY_PRIVACY_PUBLIC],
]
@onready var log_text: RichTextLabel = %LogText

const STATUS_NAMES := {
	DiscordClient.STATUS_DISCONNECTED: "Disconnected",
	DiscordClient.STATUS_CONNECTING: "Connecting",
	DiscordClient.STATUS_CONNECTED: "Connected",
	DiscordClient.STATUS_READY: "Ready",
	DiscordClient.STATUS_RECONNECTING: "Reconnecting",
	DiscordClient.STATUS_DISCONNECTING: "Disconnecting",
	DiscordClient.STATUS_HTTP_WAIT: "HTTP Wait",
}


func _ready() -> void:
	Discord.status_changed.connect(_on_status_changed)
	Discord.client_ready.connect(_on_ready)
	Discord.log_message.connect(_on_log_message)
	Discord.authorization_completed.connect(_on_authorization_completed)
	Discord.rich_presence_updated.connect(_on_rich_presence_updated)
	Discord.friend_request_sent.connect(_on_friend_request_sent)
	for entry in ACTIVITY_TYPES:
		type_option.add_item(entry[0], entry[1])
	for entry in STATUS_DISPLAY_TYPES:
		status_display_option.add_item(entry[0], entry[1])
	for entry in PARTY_PRIVACIES:
		party_privacy_option.add_item(entry[0], entry[1])
	_set_status(DiscordClient.STATUS_DISCONNECTED)


func _log(msg: String) -> void:
	log_text.append_text(msg + "\n")


func _set_status(status: int) -> void:
	status_label.text = "Status: %s" % STATUS_NAMES.get(status, str(status))


# --- Button handlers ------------------------------------------------------

func _on_initialize_pressed() -> void:
	var app_id := app_id_edit.text.strip_edges().to_int()
	if app_id == 0:
		_log("[color=red]Enter a valid Application ID first.[/color]")
		return
	Discord.initialize(app_id)
	_log("Initialized with application id %d." % app_id)


func _on_authorize_pressed() -> void:
	if not Discord.is_initialized():
		_log("[color=red]Initialize first.[/color]")
		return
	_log("Opening Discord for authorization…")
	Discord.begin_authorization()


func _on_connect_token_pressed() -> void:
	if not Discord.is_initialized():
		_log("[color=red]Initialize first.[/color]")
		return
	var token := token_edit.text.strip_edges()
	if token.is_empty():
		_log("[color=red]Enter an access token.[/color]")
		return
	_log("Connecting with provided token…")
	Discord.connect_with_token(token)


func _on_set_presence_pressed() -> void:
	# Build a full Activity from the UI. Only non-empty fields are included so the
	# update stays valid (Discord rejects e.g. an unresolvable image asset).
	var activity := {
		"type": type_option.get_selected_id(),
		"details": details_edit.text,
		"state": state_edit.text,
		"status_display_type": status_display_option.get_selected_id(),
	}

	if elapsed_check.button_pressed:
		activity["timestamps"] = {"start": int(Time.get_unix_time_from_system())}

	var party_id := party_id_edit.text.strip_edges()
	if not party_id.is_empty():
		activity["party"] = {
			"id": party_id,
			"size": int(party_size_spin.value),
			"max": int(party_max_spin.value),
			"privacy": party_privacy_option.get_selected_id(),
		}

	# Note: image values must match Art Assets configured for your app in the
	# Discord Developer Portal (or be URLs), or the whole update fails.
	var assets := {}
	if not large_image_edit.text.strip_edges().is_empty():
		assets["large_image"] = large_image_edit.text
	if not large_text_edit.text.strip_edges().is_empty():
		assets["large_text"] = large_text_edit.text
	if not small_image_edit.text.strip_edges().is_empty():
		assets["small_image"] = small_image_edit.text
	if not small_text_edit.text.strip_edges().is_empty():
		assets["small_text"] = small_text_edit.text
	if not assets.is_empty():
		activity["assets"] = assets

	var buttons := []
	for pair in [[button1_label_edit, button1_url_edit], [button2_label_edit, button2_url_edit]]:
		var label: String = pair[0].text.strip_edges()
		var url: String = pair[1].text.strip_edges()
		if not label.is_empty() and not url.is_empty():
			buttons.append({"label": label, "url": url})
	if not buttons.is_empty():
		activity["buttons"] = buttons

	Discord.set_activity(activity)
	_log("Updating rich presence via set_activity…")


func _on_clear_presence_pressed() -> void:
	Discord.clear_rich_presence()
	_log("Clearing rich presence…")


func _on_send_friend_request_pressed() -> void:
	var username := friend_edit.text.strip_edges()
	if username.is_empty():
		_log("[color=red]Enter a username.[/color]")
		return
	Discord.send_discord_friend_request(username)
	_log("Sending friend request to %s…" % username)


func _on_disconnect_pressed() -> void:
	Discord.disconnect_client()
	_log("Disconnecting…")


# --- Discord signal handlers ---------------------------------------------

func _on_status_changed(status: DiscordClient.Status, error: int, error_detail: int) -> void:
	_set_status(status)
	if error != 0:
		_log("[color=orange]Status error %d (detail %d)[/color]" % [error, error_detail])


func _on_ready() -> void:
	var user := Discord.get_current_user()
	user_label.text = "User: %s (%s)" % [user.get("display_name", "?"), user.get("id", "?")]
	_log("[color=green]Ready! Connected as %s.[/color]" % user.get("username", "?"))


func _on_log_message(message: String, severity: int) -> void:
	_log("[color=gray]SDK[%d]: %s[/color]" % [severity, message])


func _on_authorization_completed(success: bool, error: String) -> void:
	if success:
		_log("[color=green]Authorization complete.[/color]")
	else:
		_log("[color=red]Authorization failed: %s[/color]" % error)


func _on_rich_presence_updated(success: bool, error: String) -> void:
	if success:
		_log("Rich presence updated.")
	else:
		_log("[color=red]Rich presence failed: %s[/color]" % error)


func _on_friend_request_sent(success: bool, error: String) -> void:
	if success:
		_log("[color=green]Friend request sent.[/color]")
	else:
		_log("[color=red]Friend request failed: %s[/color]" % error)
