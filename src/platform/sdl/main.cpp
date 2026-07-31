#include "emu.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <atomic>
#include <string>

// SDL_ShowOpenFileDialog is asynchronous: it returns immediately and delivers
// the result to a callback that, depending on the backend, may run on ANOTHER
// thread (hence the atomic). All we need here is a blocking "pick a ROM" at
// startup, so we pump events until the callback fires.
static std::atomic<bool> dialog_done{false};
static std::string picked_file;

static void SDLCALL file_dialog_callback(void*, const char* const* filelist, int){
    // filelist == NULL    -> error
    // filelist[0] == NULL -> the user cancelled
    if (filelist && filelist[0])
        picked_file = filelist[0];
    dialog_done.store(true, std::memory_order_release);
}

static bool open_file_dialog(std::string& out){
    static const SDL_DialogFileFilter filters[] = {
        { "Game Boy ROMs", "gb;gbc" },
        { "All files", "*" },
    };
    SDL_ShowOpenFileDialog(file_dialog_callback, nullptr, nullptr,
                           filters, SDL_arraysize(filters), nullptr, false);
    while (!dialog_done.load(std::memory_order_acquire)){
        SDL_PumpEvents();
        SDL_Delay(10);
    }
    out = picked_file;
    return !out.empty();
}

int main(int argc, char **argv) {
    // The dialog needs SDL up and an event loop running. SDL_Init is
    // reference-counted, so the later call from Host::init is harmless.
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Error initialising SDL: %s\n", SDL_GetError());
        return 1;
    }
    std::string rom_path;
    if (argc < 2 && open_file_dialog(rom_path)) {
        char* new_argv[] = { argv[0], rom_path.data(), nullptr };
        return emu_run(2, new_argv);
    }
    return emu_run(argc, argv);
}
