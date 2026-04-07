#include "sync.h"
#include "controller.h"
#include <SDL3/SDL.h>
Emu_sync::Emu_sync(int& ticks, int& ticks_since_last_sync, Controller* controller) : ticks(ticks), ticks_since_last_sync(ticks_since_last_sync), controller(controller) {
    this->last_time = std::chrono::high_resolution_clock::now();
}

void Emu_sync::sync(){
    ticks_since_last_sync = 0;
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