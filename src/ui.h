#pragma once
#include <memory.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

class Ui{
    private:
        Memory& mem;
        int scale;
        SDL_Window* main_window;
        SDL_Renderer* main_renderer;
        SDL_Texture* main_texture;
        SDL_Surface* main_surface;
        SDL_Window* tile_debug_window;
        SDL_Renderer* tile_debug_renderer;
        SDL_Texture* tile_debug_texture;
        SDL_Surface* tile_debug_surface;
        bool screens_on = true;
        
        void tile_dbg_update();
        void tile_display(u16 tile, int x, int y);
        void handle_events();
    public:
        static const u32 gb_palette[4];
        Ui(Memory& mem, int scale = 4);
        bool change_requested = false;
        void init();
        void update();
        void toggle_screens();
};