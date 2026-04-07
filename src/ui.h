#pragma once
#include "utils.h"

#include "screen_specs.h"
#include "memory.h"
#include "controller.h"
#include <mutex>
#include <SDL3/SDL.h>
class Debugger;
class Ui{
    private:
        Memory& mem;
        Controller& controller;
        Debugger* dbg;
        int scale;
        SDL_Window* main_window;
        SDL_Renderer* main_renderer;
        SDL_Texture* main_texture;
        //SDL_Surface* main_surface;
        SDL_Window* tile_debug_window;
        SDL_Renderer* tile_debug_renderer;
        SDL_Texture* tile_debug_texture;
        SDL_Surface* tile_debug_surface;
        bool screens_on = true;
        u32* video_buffer;
        void tile_dbg_update();
        void main_screen_update();
        void tile_display(u16 tile, int x, int y);
        void handle_events();
        std::mutex video_buffer_mutex;
    public:
        Ui(Memory& mem, Controller& controller, int scale = 4);
        bool change_requested = false;
        void init();
        void update();
        void toggle_screens();
        void write_pixel(int x, int y, u32 color);
        void set_debugger(Debugger* dbg);
};