#include "log.h"
#include <cstdio>

// On the desktop there is a terminal behind the emulator, so the messages go
// where they always went. Errors are the only ones told apart, and only to send
// them to stderr the way the save state code already did.
class Sdl_log : public ILog {
    public:
        void write(Log_level level, const char* msg) override {
            std::fputs(msg, level == Log_level::ERROR ? stderr : stdout);
        }
};

static Sdl_log g_sdl_log;

ILog* create_log(){
    return &g_sdl_log;
}
