// This is the single translation unit that compiles the Discord Social SDK's
// header-only implementation. discordpp.h must be included with
// DISCORDPP_IMPLEMENTATION defined in EXACTLY ONE .cpp file across the whole
// build; every other file includes discordpp.h without the macro.
#define DISCORDPP_IMPLEMENTATION
#include <discordpp.h>
