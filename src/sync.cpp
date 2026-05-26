#include "sync.h"
#include "controller.h"
#include "memory.h"
#include "ui.h"
#include <SDL3/SDL.h>
Emu_sync::Emu_sync(int& ticks, int& ticks_since_last_sync, Memory* mem, Controller* controller, Ui* ui) : ticks(ticks), ticks_since_last_sync(ticks_since_last_sync), mem(mem), controller(controller), ui(ui) {
    this->last_time = std::chrono::high_resolution_clock::now();
    this->last_title_time = this->last_time;
}

void Emu_sync::sync(){
    ticks_since_last_sync = 0;
    mem->sync_mem_ui_copy();
    ui->sync_video_buffer();
    controller->process_events();

    // Speed indicator: count synced frames over ~1s windows and forward the
    // resulting percentage to the UI. Game Boy nominal rate: 59.7275 fps.
    this->frames_since_title++;
    auto now_title = std::chrono::high_resolution_clock::now();
    auto title_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now_title - this->last_title_time).count();
    if (title_elapsed_ms >= 1000){
        double expected_frames = 59.7275 * (title_elapsed_ms / 1000.0);
        int percent = (int)((this->frames_since_title / expected_frames) * 100.0 + 0.5);
        ui->set_speed_percent(percent);
        this->frames_since_title = 0;
        this->last_title_time = now_title;
    }

    if(this->turbo_mode){
        return;
    }
    auto current_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - this->last_time).count();
    //En la game boy real, 1 frame = 16.74 ms = 16740000 ns. Si pasa menos, delay_precise
    if (elapsed < 16740000){
        SDL_DelayPrecise(16740000 - elapsed);
    }
    this->last_time = std::chrono::high_resolution_clock::now();
}
void Emu_sync::set_turbo_mode(bool enabled){
    this->turbo_mode = enabled;
}
void Emu_sync::reset_speed_window(){
    this->frames_since_title = 0;
    this->last_title_time = std::chrono::high_resolution_clock::now();
}