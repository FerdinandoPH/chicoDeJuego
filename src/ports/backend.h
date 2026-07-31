#pragma once
#include "video.h"
#include "audio.h"
#include "system.h"

// Backend factories. The core declares them but does NOT implement them: the
// linker resolves them against whichever backend was compiled in (platform/sdl/,
// and any other one in the future). Switching backends means changing what gets
// linked, without touching a single line of the core.
IVideo*  create_video();
IAudio*  create_audio();
ISystem* create_system();
