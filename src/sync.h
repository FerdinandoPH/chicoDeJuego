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
        static const int audio_bytes_per_sec = 48000 * 2 * sizeof(float); // 48000 Hz, stereo, float
        static const int target_bytes = audio_bytes_per_sec * 40 / 1000;
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