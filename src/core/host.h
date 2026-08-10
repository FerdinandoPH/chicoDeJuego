#pragma once
#include "utils.h"

#include "screen_specs.h"
#include "memory.h"
#include "controller.h"
#include "video.h"
#include "audio.h"
#include "system.h"
#include <mutex>
#include <atomic>
class Run_control;

#define AUDIO_SAMPLE_BUFFER_SIZE 512

// Core-side state of a debug window. The graphics resources (window, renderer,
// texture) are owned by the backend; only what the emulator needs in order to
// decide what to draw lives here.
struct DebugWindow {
    // Opened and closed by the thread that owns the backend, but read from the
    // emulation thread too: the debug menu prints which windows are on. Relaxed
    // is enough, it is only ever consulted on its own.
    std::atomic<bool> active{false};
    int width;
    int height;
    int scale;
    const char* title;
};
typedef struct{
    u32 video_buffer[XRES*YRES];
}Host_ss;

// Aggregates everything that faces outwards: video, audio, input and time.
// It knows no concrete platform, only the ports.
class Host : public IEvent_sink {
    private:
        Memory& mem;
        Controller& controller;
        Run_control* run_control;
        int scale;
        IVideo*  video;
        IAudio*  audio;
        ISystem* sys;
        bool quit_requested = false;
        u32 video_buffer_ppu[XRES * YRES];
        u32 video_buffer_inter[XRES * YRES];
        // Third copy, only touched by the thread that presents. It exists so the
        // frame can be taken out from under video_buffer_mutex and handed to the
        // backend with the mutex already released: present() blocks until the
        // vsync, and holding the mutex across that wait stalls the emulation
        // thread on every sync_video_buffer().
        u32 video_buffer_render[XRES * YRES];
        DebugWindow debug_windows[NUM_DEBUG_WINDOWS];
        u8 mem_copy[0x10000];
        u8 vram_copy[0x4000];
        Cram cram_copy = {};
        GB_model gb_model = GB_model::DMG;
        void create_debug_window(DebugWindowType type);
        void destroy_debug_window(DebugWindowType type);
        void main_screen_update();
        void tiles_dbg_update();
        void bg_map_dbg_update();
        void win_map_dbg_update();
        void oam_dbg_update();
        void draw_dbg_tile(u32* pixel_buf, int buf_w, u16 tile_addr, int x, int y, u8 palette);
        // CGB helpers
        u32 color_cgb_to_rgb(u16 cgb_color);
        u32 cgb_color_from_cram(bool obj, u8 pal_idx, u8 color_idx);
        void draw_dbg_tile_cgb(u32* pixel_buf, int buf_w, u16 tile_addr, u8 bank,
                               int x, int y, u8 cgb_pal_idx, bool x_flip, bool y_flip);
        void close_all_debug_windows();
        std::mutex video_buffer_mutex;
        std::atomic<int> pending_speed_percent{-1}; // -1 = no percentage shown
        int shown_speed_percent = -2;               // sentinel forces first apply
        void update_speed_title();

        float audio_samples_buffer[AUDIO_SAMPLE_BUFFER_SIZE * 2];
        size_t audio_samples_buffer_index = 0;
    public:
        std::atomic<bool> debug_toggle_requested[NUM_DEBUG_WINDOWS] = {};
        Host(Memory& mem, Controller& controller, int scale = 4);
        void init();
        bool update();
        bool is_debug_window_active(DebugWindowType type);
        void write_pixel(int x, int y, u32 color);
        void set_run_control(Run_control* run_control);
        void clear_main_screen();
        void set_speed_percent(int percent);
        void clear_speed_percent();
        Host_ss save_state();
        void load_state(const Host_ss& state);
        void sync_video_buffer();

        void push_audio_sample(float left, float right);
        int get_audio_queue_size();
        void clear_audio_queue();

        bool open_with_default_app(const char* path);
        void delay_us(u64 us);

        // IEvent_sink: what the backend reports back to us.
        void on_key(Host_key k, Controller_event_type t) override;
        void on_quit() override;
        void on_break() override;
        void on_aux_closed(DebugWindowType w) override;
};
