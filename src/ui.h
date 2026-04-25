#pragma once
#include "utils.h"

#include "screen_specs.h"
#include "memory.h"
#include "controller.h"
#include <mutex>
#include <atomic>
#include <SDL3/SDL.h>

class Debugger;

#define NUM_DEBUG_WINDOWS 4

enum class DebugWindowType { TILES = 0, BG_MAP = 1, WIN_MAP = 2, OAM = 3 };

struct DebugWindow {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    bool active = false;
    int width;
    int height;
    int scale;
    const char* title;
};
typedef struct{
    u32 video_buffer[XRES*YRES];
}Ui_ss;
class Ui{
    private:
        Memory& mem;
        Controller& controller;
        Debugger* dbg;
        int scale;
        SDL_Window* main_window;
        SDL_Renderer* main_renderer;
        SDL_Texture* main_texture;
        u32 video_buffer[XRES * YRES];
        DebugWindow debug_windows[NUM_DEBUG_WINDOWS];
        u8 mem_copy[0x10000];
        void create_debug_window(DebugWindowType type);
        void destroy_debug_window(DebugWindowType type);
        void main_screen_update();
        void tiles_dbg_update();
        void bg_map_dbg_update();
        void win_map_dbg_update();
        void oam_dbg_update();
        void draw_tile(u32* pixel_buf, int buf_w, u16 tile_addr, int x, int y, u8 palette);
        bool handle_events();
        std::mutex video_buffer_mutex;
    public:
        std::atomic<bool> debug_toggle_requested[NUM_DEBUG_WINDOWS] = {};
        Ui(Memory& mem, Controller& controller, int scale = 4);
        void init();
        bool update();
        bool is_debug_window_active(DebugWindowType type);
        void write_pixel(int x, int y, u32 color);
        void set_debugger(Debugger* dbg);
        void clear_main_screen();
        Ui_ss save_state();
        void load_state(const Ui_ss& state);
};
