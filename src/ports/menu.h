#pragma once

class Debug_api;

// The debug menu. Declared by the core, implemented by the backend: the linker
// resolves it just like the factories in backend.h.
//
// It exists because a menu is input, and input is the one thing the core cannot
// assume anything about. Reading a command from the terminal only works where
// there is a terminal; a console has to draw the menu on screen and navigate it
// with the d-pad. Both drive the very same Debug_api, so the emulator gains
// commands in one place and every backend gets them.
//
// It is called from the emulation thread, which has already stopped at an
// instruction boundary, and it MUST NOT return until the user is done: returning
// is what lets emulation continue. What happens in between is entirely the
// backend's business, including handing the work over to another thread (an
// on-screen menu drawn by the render loop) and waiting here until it finishes.
void platform_debug_menu(Debug_api& api);
