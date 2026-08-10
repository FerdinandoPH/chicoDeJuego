#include "emu.h"
#include <SDL3/SDL_main.h>

// The entry point belongs to the backend: each one has its own (libretro does not
// even have a main).
//
// Asking the user for a ROM used to happen here, before emu_run. It does not any
// more: the dialog has to be owned by the emulator's window, and that window only
// exists once the core has called Host::init(). So the core asks for the ROM
// itself through ISystem::pick_file, and Sdl_system parents the dialog properly.
// Doing it here left the process with no window while the dialog was up, and on
// Windows that meant the main window opened behind every other application.
int main(int argc, char **argv) {
    return emu_run(argc, argv);
}
