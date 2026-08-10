#include "host.h"
#include "backend.h"
#include "run_control.h"
#include "ppu.h"   // Sprite, for the OAM viewer
#include <cstdio>

static const struct { int w; int h; int scale; const char* title; } dbg_window_info[NUM_DEBUG_WINDOWS] = {
    { TILE_DBG_W,    TILE_DBG_H,    3, "Tile Viewer"  },
    { BG_MAP_DBG_W,  BG_MAP_DBG_H,  2, "BG Map"       },
    { WIN_MAP_DBG_W, WIN_MAP_DBG_H, 2, "Window Map"   },
    { OAM_DBG_W,     OAM_DBG_H,     3, "OAM Sprites"  },
};

Host::Host(Memory& mem, Controller& controller, int scale) :  mem(mem), controller(controller), scale(scale), video_buffer_mutex() {
    // The linker resolves these factories against whichever backend is built.
    this->video = create_video();
    this->audio = create_audio();
    this->sys   = create_system();
    for (int i = 0; i < NUM_DEBUG_WINDOWS; i++){
        debug_windows[i].width  = dbg_window_info[i].w;
        debug_windows[i].height = dbg_window_info[i].h;
        debug_windows[i].scale  = dbg_window_info[i].scale;
        debug_windows[i].title  = dbg_window_info[i].title;
    }
}
void Host::init(){
    this->video->init(XRES, YRES, this->scale);
    this->audio->init(48000, 2);
    // The framebuffers start out as whatever was in memory, and the window is
    // visible from now on (the ROM picker runs with it already up), so give it
    // something to show.
    this->clear_main_screen();
}
void Host::set_run_control(Run_control* run_control){
    this->run_control = run_control;
}

// --- Debug window lifecycle ---

void Host::create_debug_window(DebugWindowType type){
    int i = (int)type;
    DebugWindow& dw = debug_windows[i];
    if (dw.active.load(std::memory_order_relaxed)) return;

    // The Tile Viewer shows both VRAM banks in CGB mode (bank 0 left, bank 1
    // right), so it needs double width. This is Game Boy knowledge, so it stays
    // here and the already-computed width is handed to the backend.
    if (type == DebugWindowType::TILES)
        dw.width = (mem.get_gb_model() == GB_model::CGB) ? (TILE_DBG_W * 2) : TILE_DBG_W;

    video->open_aux(type, dw.width, dw.height, dw.scale, dw.title);
    dw.active.store(true, std::memory_order_relaxed);
}
void Host::destroy_debug_window(DebugWindowType type){
    DebugWindow& dw = debug_windows[(int)type];
    if (!dw.active.load(std::memory_order_relaxed)) return;

    video->close_aux(type);
    dw.active.store(false, std::memory_order_relaxed);
}
void Host::close_all_debug_windows(){
    for (int i = 0; i < NUM_DEBUG_WINDOWS; i++)
        destroy_debug_window((DebugWindowType)i);
}
bool Host::is_debug_window_active(DebugWindowType type){
    return debug_windows[(int)type].active.load(std::memory_order_relaxed);
}

// --- Events ---
// The pumping itself lives in the backend (pump_events). Only the reactions are
// here, and they are the very same lines that used to sit inline inside the SDL
// switch.

void Host::on_key(Host_key k, Controller_event_type t){
    this->controller.enqueue_event(k, t);
}
void Host::on_quit(){
    this->quit_requested = true;
}
void Host::on_break(){
    // Only a request: the emulation thread stops itself when it reaches a point
    // where stopping is safe. Poking the debug level from here would be a race.
    this->run_control->request_break();
}
void Host::on_aux_closed(DebugWindowType w){
    // The user closed the window from the window manager: release it just as if
    // it had been toggled off from the debug menu.
    destroy_debug_window(w);
}

// --- Update ---

bool Host::update() {
    // Process pending debug window toggles
    for (int i = 0; i < NUM_DEBUG_WINDOWS; i++){
        if (debug_toggle_requested[i]){
            debug_toggle_requested[i] = false;
            if (debug_windows[i].active.load(std::memory_order_relaxed))
                destroy_debug_window((DebugWindowType)i);
            else
                create_debug_window((DebugWindowType)i);
        }
    }

    this->sys->pump_events(*this);
    if (this->quit_requested){
        close_all_debug_windows();
        this->video->shutdown();
        this->audio->shutdown();
        return false;
    }

    this->update_speed_title();
    this->mem.get_mem_ui_copy(this->mem_copy);
    this->mem.get_vram_ui_copy(this->vram_copy);
    this->mem.get_cram_ui_copy(&this->cram_copy);
    this->gb_model = this->mem.get_gb_model();
    this->main_screen_update();
    this->tiles_dbg_update();
    this->bg_map_dbg_update();
    this->win_map_dbg_update();
    this->oam_dbg_update();
    return true;
}

// --- Main screen ---
void Host::clear_main_screen(){
    {
        std::scoped_lock<std::mutex> lock(video_buffer_mutex);
        std::fill_n(this->video_buffer_inter, XRES * YRES, 0xFF000000); // Clear to black
        memcpy(this->video_buffer_render, this->video_buffer_inter, sizeof(this->video_buffer_inter));
    }
    video->present(this->video_buffer_render);
}
void Host::clear_video_buffers(){
    std::scoped_lock<std::mutex> lock(video_buffer_mutex);
    std::fill_n(this->video_buffer_ppu, XRES * YRES, 0xFF000000);
    std::fill_n(this->video_buffer_inter, XRES * YRES, 0xFF000000);
}
void Host::write_pixel(int x, int y, u32 color){
    if (x < 0 || x >= XRES || y < 0 || y >= YRES) return;
    this->video_buffer_ppu[y * XRES + x] = color;
}
void Host::sync_video_buffer(){
    std::scoped_lock<std::mutex> lock(video_buffer_mutex);
    memcpy(this->video_buffer_inter, this->video_buffer_ppu, sizeof(this->video_buffer_ppu));
}
void Host::main_screen_update(){
    // The mutex only has to protect video_buffer_render from being rewritten by
    // sync_video_buffer() while we read it. present() is left outside on purpose:
    // it waits for the vsync, and the emulation thread needs this mutex once per
    // frame.
    {
        std::scoped_lock<std::mutex> lock(video_buffer_mutex);
        memcpy(this->video_buffer_render, this->video_buffer_inter, sizeof(this->video_buffer_inter));
    }
    video->present(this->video_buffer_render);
}

// --- Debug window update stubs (rendering logic will be added later) ---

void Host::draw_dbg_tile(u32* pixel_buf, int buf_w, u16 tile_addr, int x, int y, u8 palette){
    /*
    draw_tile
    Parameters:
        pixel_buf: pointer to the buffer where the tile's pixels will be drawn. Format is ARGB8888.
        buf_w: width of the pixel_buf in pixels (used for indexing)
        mem: pointer to the beginning of memory (used to read tile data)
        tile_addr: address in memory where the tile data starts
        x, y: coordinates in pixel_buf where the top-left corner of the tile will be drawn
    */
    for (int row = 0; row < 8; row++){
        u8 lo = mem_copy[tile_addr + row * 2];
        u8 hi = mem_copy[tile_addr + row * 2 + 1];
        for (int col = 0; col < 8; col++){
            int bit = 7 - col;
            int color_id = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);
            color_id = (palette >> (color_id * 2)) & 0x3; // Apply palette
            pixel_buf[(y + row) * buf_w + (x + col)] = gb_palette[color_id];
        }
    }
}

// Converts a 15-bit CGB color (BGR555) to ARGB8888. Mirrors the PPU renderer.
u32 Host::color_cgb_to_rgb(u16 cgb_color){
    u8 r5 = cgb_color & 0x1F;
    u8 g5 = (cgb_color >> 5) & 0x1F;
    u8 b5 = (cgb_color >> 10) & 0x1F;
    u8 r8 = (r5 << 3) | (r5 >> 2);
    u8 g8 = (g5 << 3) | (g5 >> 2);
    u8 b8 = (b5 << 3) | (b5 >> 2);
    return (0xFFu << 24) | (r8 << 16) | (g8 << 8) | b8;
}

// Resolves a color from the CGB palette RAM snapshot (bg or obj palettes).
u32 Host::cgb_color_from_cram(bool obj, u8 pal_idx, u8 color_idx){
    pal_idx &= 7;
    color_idx &= 3;
    const u8* palettes = obj ? cram_copy.obj_palettes : cram_copy.bg_palettes;
    u8 offset = pal_idx * 8 + color_idx * 2;
    u16 color = (u16)palettes[offset] | ((u16)palettes[offset + 1] << 8);
    return color_cgb_to_rgb(color);
}

// Draws an 8x8 tile using CGB attributes: reads tile data from the given VRAM
// bank, applies the BG/Win CGB palette from CRAM, and honors x/y flip.
void Host::draw_dbg_tile_cgb(u32* pixel_buf, int buf_w, u16 tile_addr, u8 bank,
                           int x, int y, u8 cgb_pal_idx, bool x_flip, bool y_flip){
    for (int row = 0; row < 8; row++){
        int src_row = y_flip ? (7 - row) : row;
        u32 base = (u32)(tile_addr - 0x8000) + bank * 0x2000 + src_row * 2;
        u8 lo = vram_copy[base];
        u8 hi = vram_copy[base + 1];
        for (int col = 0; col < 8; col++){
            int bit = x_flip ? col : (7 - col);
            int color_id = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);
            pixel_buf[(y + row) * buf_w + (x + col)] = cgb_color_from_cram(false, cgb_pal_idx, color_id);
        }
    }
}

void Host::tiles_dbg_update(){
    DebugWindow& dw = debug_windows[(int)DebugWindowType::TILES];
    if (!dw.active.load(std::memory_order_relaxed)) return;

    bool cgb = (gb_model == GB_model::CGB);
    // In CGB both VRAM banks (2 x 384 tiles) are shown side by side, so the
    // buffer may be twice as wide. The window/texture size is set to match in
    // create_debug_window.
    int buf_w = dw.width;
    int buf_h = dw.height;
    u32 pixel_buf[(TILE_DBG_W * 2) * TILE_DBG_H]; // max size (both banks)
    std::fill_n(pixel_buf, buf_w * buf_h, GRID_COLOR);

    // Tiles have no inherent palette, so CGB tiles are shown with a neutral gray ramp.
    static const u32 gray_ramp[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};

    int num_tiles = cgb ? 768 : 384;
    for (int tile_idx = 0; tile_idx < num_tiles; tile_idx++){
        u8 bank        = tile_idx / 384;   // 0 = VRAM bank 0, 1 = VRAM bank 1
        int local_idx  = tile_idx % 384;
        u16 tile_addr  = 0x8000 + local_idx * 16;
        // Bank 0 fills the left 16 columns, bank 1 the right 16 columns.
        int x = (bank * 16 + (local_idx % 16)) * 9;
        int y = (local_idx / 16) * 9;
        if (cgb){
            for (int row = 0; row < 8; row++){
                u32 base = (u32)(tile_addr - 0x8000) + bank * 0x2000 + row * 2;
                u8 lo = vram_copy[base];
                u8 hi = vram_copy[base + 1];
                for (int col = 0; col < 8; col++){
                    int bit = 7 - col;
                    int color_id = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);
                    pixel_buf[(y + row) * buf_w + (x + col)] = gray_ramp[color_id];
                }
            }
        } else {
            draw_dbg_tile(pixel_buf, buf_w, tile_addr, x, y, 0b11100100);
        }
    }

    video->present_aux(DebugWindowType::TILES, pixel_buf, buf_w);
}
void Host::bg_map_dbg_update(){
    DebugWindow& dw = debug_windows[(int)DebugWindowType::BG_MAP];
    if (!dw.active.load(std::memory_order_relaxed)) return;
    u32 pixel_buf[BG_MAP_DBG_W * BG_MAP_DBG_H];
    std::fill_n(pixel_buf, BG_MAP_DBG_W * BG_MAP_DBG_H, GRID_COLOR);
    bool cgb = (gb_model == GB_model::CGB);
    u16 bg_map_base_addr = mem_copy[LCDC_ADDR] & 0x8 ? 0x9C00 : 0x9800;
    u16 tile_data_base = mem_copy[LCDC_ADDR] & 0x10 ? 0x8000 : 0x9000;
    for (int i=0; i < 1024; i++){
        // Tile indices always live in VRAM bank 0; in CGB the currently mapped
        // bank in mem_copy is unreliable, so read from the VRAM snapshot directly.
        u16 tile_idx = cgb ? vram_copy[bg_map_base_addr - 0x8000 + i]
                           : mem_copy[bg_map_base_addr + i];
        u16 tile_addr = tile_data_base;
        if(tile_data_base == 0x9000)
            tile_addr += (i8)tile_idx * 16; // Signed index
        else
            tile_addr += tile_idx * 16;      // Unsigned index
        int x = (i % 32) * 9;
        int y = (i / 32) * 9;
        if (cgb){
            u8 attr = vram_copy[bg_map_base_addr - 0x8000 + 0x2000 + i]; // attrs in bank 1
            draw_dbg_tile_cgb(pixel_buf, BG_MAP_DBG_W, tile_addr, (attr & 0x8) >> 3,
                              x, y, attr & 0x7, attr & 0x20, attr & 0x40);
        } else {
            draw_dbg_tile(pixel_buf, BG_MAP_DBG_W, tile_addr, x, y, mem_copy[BGP_ADDR]);
        }
    }

    // --- Viewport rectangle ---
    // The BG map is 256x256 pixels (32x32 tiles). The Game Boy screen is a
    // 160x144 window into that map, anchored at (SCX, SCY). Since the map
    // wraps around (it's a torus), the rectangle may cross the right or
    // bottom edge and reappear on the opposite side.
    u8 scx = mem_copy[SCX_ADDR];
    u8 scy = mem_copy[SCY_ADDR];
    const u32 VIEWPORT_COLOR = 0xFF00AAFF;

    // Convert a GB map coordinate (0-255) to a pixel index in the debug buffer.
    // The debug window draws each 8-pixel tile as a 9-pixel cell (8 data pixels
    // + 1 grid pixel), so coordinate `gb` lives at column/row:
    //     (gb / 8) * 9  -> base of the tile cell
    //     + (gb % 8)    -> offset inside that tile
    auto gb_to_dbg = [](int gb){ return (gb / 8) * 9 + (gb % 8); };

    // Top and bottom edges: iterate horizontally across the 160-pixel width.
    // For each column dx in screen space, the corresponding map X is
    // (scx + dx) mod 256. The top edge sits at map Y = scy, and the bottom
    // edge at scy + 143 (also wrapped).
    for (int dx = 0; dx < XRES; dx++){
        int bx = gb_to_dbg((scx + dx) % 256);
        pixel_buf[gb_to_dbg(scy % 256)             * BG_MAP_DBG_W + bx] = VIEWPORT_COLOR; // top
        pixel_buf[gb_to_dbg((scy + YRES - 1) % 256) * BG_MAP_DBG_W + bx] = VIEWPORT_COLOR; // bottom
    }

    // Left and right edges: iterate vertically across the 144-pixel height.
    // Symmetric to the loop above, but swapping the role of X and Y.
    for (int dy = 0; dy < YRES; dy++){
        int by = gb_to_dbg((scy + dy) % 256);
        pixel_buf[by * BG_MAP_DBG_W + gb_to_dbg(scx % 256)]             = VIEWPORT_COLOR; // left
        pixel_buf[by * BG_MAP_DBG_W + gb_to_dbg((scx + XRES - 1) % 256)] = VIEWPORT_COLOR; // right
    }

    video->present_aux(DebugWindowType::BG_MAP, pixel_buf, BG_MAP_DBG_W);
}
void Host::win_map_dbg_update(){
    DebugWindow& dw = debug_windows[(int)DebugWindowType::WIN_MAP];
    if (!dw.active.load(std::memory_order_relaxed)) return;
    u32 pixel_buf[WIN_MAP_DBG_W * WIN_MAP_DBG_H];
    std::fill_n(pixel_buf, WIN_MAP_DBG_W * WIN_MAP_DBG_H, GRID_COLOR);
    bool cgb = (gb_model == GB_model::CGB);
    u16 win_map_base_addr = mem_copy[LCDC_ADDR] & 0x40 ? 0x9C00 : 0x9800;
    u16 tile_data_base = mem_copy[LCDC_ADDR] & 0x10 ? 0x8000 : 0x9000;
    for (int i=0; i < 1024; i++){
        u16 tile_idx = cgb ? vram_copy[win_map_base_addr - 0x8000 + i]
                           : mem_copy[win_map_base_addr + i];
        u16 tile_addr = tile_data_base;
        if(tile_data_base == 0x9000)
            tile_addr += (i8)tile_idx * 16; // Signed index
        else
            tile_addr += tile_idx * 16;      // Unsigned index
        int x = (i % 32) * 9;
        int y = (i / 32) * 9;
        if (cgb){
            u8 attr = vram_copy[win_map_base_addr - 0x8000 + 0x2000 + i]; // attrs in bank 1
            draw_dbg_tile_cgb(pixel_buf, WIN_MAP_DBG_W, tile_addr, (attr & 0x8) >> 3,
                              x, y, attr & 0x7, attr & 0x20, attr & 0x40);
        } else {
            draw_dbg_tile(pixel_buf, WIN_MAP_DBG_W, tile_addr, x, y, mem_copy[BGP_ADDR]);
        }
    }
    video->present_aux(DebugWindowType::WIN_MAP, pixel_buf, WIN_MAP_DBG_W);
}
void Host::oam_dbg_update(){
    DebugWindow& dw = debug_windows[(int)DebugWindowType::OAM];
    if (!dw.active.load(std::memory_order_relaxed)) return;

    u32 pixel_buf[OAM_DBG_W * OAM_DBG_H];
    std::fill_n(pixel_buf, OAM_DBG_W * OAM_DBG_H, GRID_COLOR);

    bool cgb      = (gb_model == GB_model::CGB);
    bool tall     = mem_copy[LCDC_ADDR] & 0x04;
    int sprite_h  = tall ? 16 : 8;

    for (int i = 0; i < 40; i++){
        Sprite spr(i, &mem_copy[0xFE00 + i * 4]);

        u8 tile_index = spr.tile_index;
        if (tall) tile_index &= 0xFE;
        u16 tile_addr = 0x8000 + tile_index * 16;
        u8 palette    = mem_copy[spr.dmg_palette ? OBP1_ADDR : OBP0_ADDR];
        u8 bank       = cgb ? spr.cgb_bank : 0;

        int px = (i % 10) * 9;
        int py = (i / 10) * 17;

        for (int r = 0; r < sprite_h; r++){
            int src_row  = spr.y_flip ? (sprite_h - 1 - r) : r;
            u16 row_addr = tile_addr + src_row * 2;
            u8 lo, hi;
            if (cgb){
                u32 base = (u32)(row_addr - 0x8000) + bank * 0x2000;
                lo = vram_copy[base];
                hi = vram_copy[base + 1];
            } else {
                lo = mem_copy[row_addr];
                hi = mem_copy[row_addr + 1];
            }
            for (int c = 0; c < 8; c++){
                int bit      = spr.x_flip ? c : (7 - c);
                int color_id = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);
                u32 color;
                if (cgb){
                    color = cgb_color_from_cram(true, spr.cgb_palette_index, color_id);
                } else {
                    int mapped = (palette >> (color_id * 2)) & 0x3;
                    color = gb_palette[mapped];
                }
                pixel_buf[(py + r) * OAM_DBG_W + (px + c)] = color;
            }
        }
    }

    video->present_aux(DebugWindowType::OAM, pixel_buf, OAM_DBG_W);
}

void Host::set_speed_percent(int percent){
    this->pending_speed_percent.store(percent, std::memory_order_relaxed);
}
void Host::clear_speed_percent(){
    this->pending_speed_percent.store(-1, std::memory_order_relaxed);
}
void Host::update_speed_title(){
    int p = this->pending_speed_percent.load(std::memory_order_relaxed);
    if (p == this->shown_speed_percent) return;
    this->shown_speed_percent = p;
    if (p < 0){
        video->set_title("Chico de Juego");
    } else {
        char title[64];
        std::snprintf(title, sizeof(title), "Chico de Juego - %d%%", p);
        video->set_title(title);
    }
}

void Host::push_audio_sample(float left, float right){
    audio_samples_buffer[audio_samples_buffer_index++] = left;
    audio_samples_buffer[audio_samples_buffer_index++] = right;
    if (audio_samples_buffer_index >= AUDIO_SAMPLE_BUFFER_SIZE * 2){
        audio->submit(audio_samples_buffer, AUDIO_SAMPLE_BUFFER_SIZE * 2);
        audio_samples_buffer_index = 0;
    }
}
int Host::get_audio_queue_size(){
    return audio->queued_bytes();
}
void Host::clear_audio_queue(){
    audio->clear();
}

Host_ss Host::save_state(){
    Host_ss state;
    {
        std::scoped_lock<std::mutex> lock(video_buffer_mutex);
        std::copy(std::begin(this->video_buffer_ppu), std::end(this->video_buffer_ppu), std::begin(state.video_buffer));
    }
    return state;
}
void Host::load_state(const Host_ss& state){
    {
        std::scoped_lock<std::mutex> lock(video_buffer_mutex);
        std::copy(std::begin(state.video_buffer), std::end(state.video_buffer), std::begin(this->video_buffer_ppu));
        memcpy(this->video_buffer_render, this->video_buffer_ppu, sizeof(this->video_buffer_ppu));
    }
    video->present(this->video_buffer_render);
}
bool Host::open_with_default_app(const char* path){
    return sys->open_with_default_app(path);
}
bool Host::pick_file(const char* title, const char* filter_name,
                     const char* filter_pattern, std::string& out){
    return sys->pick_file(title, filter_name, filter_pattern, out);
}
// The parameter is microseconds: it used to be named "ms" by mistake, even
// though the arithmetic (us * 1000 = ns) was always right.
void Host::delay_us(u64 us){
    sys->delay_ns(us * 1000);
}