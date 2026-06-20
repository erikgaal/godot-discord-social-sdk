extends SceneTree

## Self-contained headless test runner for the Discord Social SDK extension.
## No external dependencies — run with:
##
##   godot --headless --path demo --script res://tests/run_tests.gd
##
## Exits with code 0 on success, 1 on any failure (suitable for CI).
##
## These tests exercise what can be checked without a live Discord connection:
## class registration, the method/signal/enum surface, and default state.
## Anything requiring an authenticated session is out of scope for automated
## tests and is covered by the interactive demo instead.

var _passed := 0
var _failed := 0


func _init() -> void:
	print("== Discord Social SDK tests ==")

	_test_class_registered()
	_test_instantiable_node()
	_test_methods_present()
	_test_signals_present()
	_test_enums_present()
	_test_default_state()
	_test_uninitialized_calls_are_safe()

	print("\n== %d passed, %d failed ==" % [_passed, _failed])
	quit(1 if _failed > 0 else 0)


func _check(condition: bool, label: String) -> void:
	if condition:
		_passed += 1
		print("  [PASS] %s" % label)
	else:
		_failed += 1
		print("  [FAIL] %s" % label)


func _test_class_registered() -> void:
	_check(ClassDB.class_exists("DiscordClient"), "DiscordClient is registered")
	_check(ClassDB.get_parent_class("DiscordClient") == "Node", "DiscordClient extends Node")


func _test_instantiable_node() -> void:
	var inst = ClassDB.instantiate("DiscordClient")
	_check(inst != null, "DiscordClient instantiates")
	_check(inst is Node, "instance is a Node")
	if inst:
		inst.free()


func _test_methods_present() -> void:
	var inst = ClassDB.instantiate("DiscordClient")
	for m in [
		"initialize", "is_initialized", "connect_with_token", "begin_authorization",
		"request_authorization_code",
		"set_game_window_pid", "disconnect_client", "set_rich_presence", "set_activity",
		"clear_rich_presence", "send_discord_friend_request", "get_current_user",
		"get_status", "get_sdk_version", "poll",
	]:
		_check(inst.has_method(m), "method %s exists" % m)
	inst.free()


func _test_signals_present() -> void:
	var inst = ClassDB.instantiate("DiscordClient")
	var names = inst.get_signal_list().map(func(s): return s.name)
	for s in [
		"status_changed", "client_ready", "log_message",
		"authorization_completed", "authorization_code_received",
		"rich_presence_updated", "friend_request_sent",
	]:
		_check(s in names, "signal %s exists" % s)
	inst.free()


func _test_enums_present() -> void:
	var inst = ClassDB.instantiate("DiscordClient")
	_check(inst.STATUS_DISCONNECTED == 0, "STATUS_DISCONNECTED == 0")
	_check(inst.STATUS_READY == 3, "STATUS_READY == 3")
	_check(inst.STATUS_HTTP_WAIT == 6, "STATUS_HTTP_WAIT == 6")
	_check(inst.ACTIVITY_PLAYING == 0, "ACTIVITY_PLAYING == 0")
	_check(inst.ACTIVITY_COMPETING == 5, "ACTIVITY_COMPETING == 5")
	inst.free()


func _test_default_state() -> void:
	var inst = ClassDB.instantiate("DiscordClient")
	_check(inst.is_initialized() == false, "starts uninitialized")
	_check(inst.get_status() == inst.STATUS_DISCONNECTED, "starts disconnected")
	_check(inst.get_current_user().is_empty(), "no current user before connect")
	var ver = inst.get_sdk_version()
	_check(ver.has("string") and int(ver.get("major", 0)) >= 1, "reports SDK version (%s)" % ver.get("string", "?"))
	inst.free()


func _test_uninitialized_calls_are_safe() -> void:
	# Calling methods before initialize() must not crash (they error-guard).
	var inst = ClassDB.instantiate("DiscordClient")
	inst.poll()
	inst.disconnect_client()
	_check(inst.get_current_user().is_empty(), "get_current_user safe when uninitialized")
	inst.free()
	_check(true, "uninitialized calls did not crash")
