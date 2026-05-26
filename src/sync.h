#pragma once
#include "utils.h"
#include <chrono>

class Controller;
class Memory;
class Ui;
class Emu_sync{
    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> last_time;
        std::chrono::time_point<std::chrono::high_resolution_clock> last_title_time;
        int frames_since_title = 0;
        int& ticks;
        int& ticks_since_last_sync;
        Memory* mem;
        Controller* controller;
        Ui* ui;
        bool turbo_mode = false;
    public:
        Emu_sync(int& ticks, int& ticks_since_last_sync, Memory* mem, Controller* controller, Ui* ui);
        void sync();
        void set_turbo_mode(bool enabled);
        void reset_speed_window();
};