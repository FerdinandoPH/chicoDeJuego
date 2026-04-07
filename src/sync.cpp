#include "sync.h"
#include "controller.h"
#include <SDL3/SDL.h>
Emu_Sync::Emu_Sync(int& ticks, int& ticks_since_last_sync, Controller* controller) : ticks(ticks), ticks_since_last_sync(ticks_since_last_sync), controller(controller) {
    this->last_time = std::chrono::high_resolution_clock::now();
}

void Emu_Sync::sync(){
    auto current_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(current_time - this->last_time).count();
    //En la game boy real, 1 frame = 16.74 ms = 1674000us
    if(elapsed < 1674000){
        
    }
}