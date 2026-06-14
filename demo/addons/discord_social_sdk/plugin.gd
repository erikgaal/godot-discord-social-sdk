@tool
extends EditorPlugin

## Registers a "Discord" autoload singleton (a DiscordClient node) so any
## script can access it as `Discord`. Disable the plugin to remove it, or
## instance DiscordClient yourself if you prefer not to use the singleton.

const AUTOLOAD_NAME := "Discord"
const AUTOLOAD_PATH := "res://addons/discord_social_sdk/discord_autoload.gd"


func _enter_tree() -> void:
	add_autoload_singleton(AUTOLOAD_NAME, AUTOLOAD_PATH)


func _exit_tree() -> void:
	remove_autoload_singleton(AUTOLOAD_NAME)
