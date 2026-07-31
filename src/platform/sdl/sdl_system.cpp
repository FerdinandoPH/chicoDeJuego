#include "system.h"
#include "sdl_internal.h"
#include <SDL3/SDL.h>

// Translation from the native key code to the core's enum. This is the only
// place in the program that knows about both SDL_Scancode and Host_key.
static Host_key sdl_scancode_to_host_key(SDL_Scancode sc){
    switch (sc){
        case SDL_SCANCODE_UP:        return Host_key::UP;
        case SDL_SCANCODE_DOWN:      return Host_key::DOWN;
        case SDL_SCANCODE_LEFT:      return Host_key::LEFT;
        case SDL_SCANCODE_RIGHT:     return Host_key::RIGHT;
        case SDL_SCANCODE_Z:         return Host_key::Z;
        case SDL_SCANCODE_X:         return Host_key::X;
        case SDL_SCANCODE_RETURN:    return Host_key::RETURN;
        case SDL_SCANCODE_BACKSPACE: return Host_key::BACKSPACE;
        case SDL_SCANCODE_SPACE:     return Host_key::SPACE;
        case SDL_SCANCODE_F1:        return Host_key::F1;
        case SDL_SCANCODE_F2:        return Host_key::F2;
        case SDL_SCANCODE_ESCAPE:    return Host_key::ESCAPE;
        default:                     return Host_key::UNKNOWN;
    }
}

class Sdl_system : public ISystem {
    public:
        void delay_ns(u64 ns) override {
            SDL_DelayPrecise(ns);
        }

        bool open_with_default_app(const char* path) override {
            return SDL_OpenURL(path);
        }

        // This used to be Ui::handle_events. The only change is the actions that
        // poked the core inline: they are now reported through the sink.
        void pump_events(IEvent_sink& sink) override {
            SDL_Event event;
            while (SDL_PollEvent(&event) > 0){
                switch (event.type){
                    case SDL_EVENT_QUIT:
                        sink.on_quit();
                        return;

                    case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                        SDL_WindowID windowID = event.window.windowID;

                        // Check if a debug window was closed
                        bool was_debug = false;
                        for (int i = 0; i < NUM_DEBUG_WINDOWS; i++){
                            if (sdl_aux_window_id(i) == windowID){
                                sink.on_aux_closed((DebugWindowType)i);
                                was_debug = true;
                                break;
                            }
                        }
                        if (was_debug) break;

                        // Main window closed -> quit
                        if (windowID == sdl_main_window_id()){
                            sink.on_quit();
                            return;
                        }
                        break;
                    }
                    case SDL_EVENT_KEY_DOWN:
                        switch (event.key.scancode){
                            case SDL_SCANCODE_ESCAPE:
                                sink.on_break();
                                break;
                            default:
                                if (!event.key.repeat)
                                    sink.on_key(sdl_scancode_to_host_key(event.key.scancode),
                                                Controller_event_type::KEY_DOWN);
                                break;
                        }
                        break;
                    case SDL_EVENT_KEY_UP:
                        if (!event.key.repeat)
                            sink.on_key(sdl_scancode_to_host_key(event.key.scancode),
                                        Controller_event_type::KEY_UP);
                        break;
                    default:
                        break;
                }
            }
        }
};

static Sdl_system g_sdl_system;

ISystem* create_system(){
    return &g_sdl_system;
}
