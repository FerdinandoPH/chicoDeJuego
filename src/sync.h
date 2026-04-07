#pragma once
#include "utils.h"
#include <chrono>

class Controller;
class Emu_Sync{
    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> last_time;
        int& ticks;
        int& ticks_since_last_sync;
        Controller* controller;
    public:
        Emu_Sync(int& ticks, int& ticks_since_last_sync, Controller* controller);
        void sync();
};