#pragma once
#include "utils.h"
#include <chrono>

class Controller;
class Emu_sync{
    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> last_time;
        int& ticks;
        int& ticks_since_last_sync;
        Controller* controller;
        bool turbo_mode = false;
    public:
        Emu_sync(int& ticks, int& ticks_since_last_sync, Controller* controller);
        void sync();
        void set_turbo_mode(bool enabled);
};