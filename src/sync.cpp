#include "sync.h"
#include "controller.h"
#include "memory.h"
#include "ui.h"
#include <SDL3/SDL.h>
Emu_sync::Emu_sync(int& ticks, int& ticks_since_last_sync, Memory* mem, Controller* controller, Ui* ui) : ticks(ticks), ticks_since_last_sync(ticks_since_last_sync), mem(mem), controller(controller), ui(ui) {
    this->last_time = std::chrono::high_resolution_clock::now();
}

void Emu_sync::sync(){
    ticks_since_last_sync = 0;
    mem->sync_mem_ui_copy();
    controller->process_events();
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