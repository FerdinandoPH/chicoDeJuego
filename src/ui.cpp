#include "ui.h"
#include "debugger.h"

static const struct { int w; int h; int scale; const char* title; } dbg_window_info[NUM_DEBUG_WINDOWS] = {
    { TILE_DBG_W,    TILE_DBG_H,    3, "Tile Viewer"  },
    { BG_MAP_DBG_W,  BG_MAP_DBG_H,  2, "BG Map"       },
    { WIN_MAP_DBG_W, WIN_MAP_DBG_H, 2, "Window Map"   },
    { OAM_DBG_W,     OAM_DBG_H,     3, "OAM Sprites"  },
};

Ui::Ui(Memory& mem, Controller& controller, int scale) :  mem(mem), controller(controller), scale(scale), video_buffer_mutex() {
    for (int i = 0; i < NUM_DEBUG_WINDOWS; i++){
        debug_windows[i].width  = dbg_window_info[i].w;
        debug_windows[i].height = dbg_window_info[i].h;
        debug_windows[i].scale  = dbg_window_info[i].scale;
        debug_windows[i].title  = dbg_window_info[i].title;
    }
}
void Ui::init(){
    SDL_CreateWindowAndRenderer("Chico de Juego", XRES*scale, YRES*scale, 0, &this->main_window, &this->main_renderer);
    SDL_SetWindowTitle(this->main_window, "Chico de Juego");
    this->main_texture = SDL_CreateTexture(this->main_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, XRES, YRES);
    SDL_SetRenderVSync(this->main_renderer, 1);
    SDL_SetTextureScaleMode(this->main_texture, SDL_SCALEMODE_NEAREST);
    SDL_SetRenderLogicalPresentation(this->main_renderer, XRES, YRES, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
}
void Ui::set_debugger(Debugger* dbg){
    this->dbg = dbg;
}

// --- Debug window lifecycle ---

void Ui::create_debug_window(DebugWindowType type){
    int i = (int)type;
    DebugWindow& dw = debug_windows[i];
    if (dw.active) return;

    SDL_CreateWindowAndRenderer(dw.title,
        dw.width * dw.scale, dw.height * dw.scale,
        0, &dw.window, &dw.renderer);
    dw.texture = SDL_CreateTexture(dw.renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        dw.width, dw.height);
    SDL_SetTextureScaleMode(dw.texture, SDL_SCALEMODE_NEAREST);
    SDL_SetRenderLogicalPresentation(dw.renderer,
        dw.width, dw.height, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

    int main_x, main_y, main_w, main_h;
    SDL_GetWindowPosition(main_window, &main_x, &main_y);
    SDL_GetWindowSize(main_window, &main_w, &main_h);
    int y = main_y;
    for (int j = 0; j < i; j++)
        if (debug_windows[j].active)
            y += debug_windows[j].height * debug_windows[j].scale;
    SDL_SetWindowPosition(dw.window, main_x + main_w, y);

    dw.active = true;
}
void Ui::destroy_debug_window(DebugWindowType type){
    int i = (int)type;
    DebugWindow& dw = debug_windows[i];
    if (!dw.active) return;

    SDL_DestroyTexture(dw.texture);
    SDL_DestroyRenderer(dw.renderer);
    SDL_DestroyWindow(dw.window);
    dw.texture  = nullptr;
    dw.renderer = nullptr;
    dw.window   = nullptr;
    dw.active   = false;
}
bool Ui::is_debug_window_active(DebugWindowType type){
    return debug_windows[(int)type].active;
}

// --- Events ---

bool Ui::handle_events(){
    SDL_Event event;
    while (SDL_PollEvent(&event) > 0){
        switch (event.type){
            case SDL_EVENT_QUIT:
                for (int i = 0; i < NUM_DEBUG_WINDOWS; i++)
                    destroy_debug_window((DebugWindowType)i);
                SDL_DestroyWindow(this->main_window);
                SDL_DestroyRenderer(this->main_renderer);
                SDL_DestroyTexture(this->main_texture);
                return false;

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
                Uint32 windowID = event.window.windowID;

                // Check if a debug window was closed
                bool was_debug = false;
                for (int i = 0; i < NUM_DEBUG_WINDOWS; i++){
                    if (debug_windows[i].active &&
                        SDL_GetWindowID(debug_windows[i].window) == windowID){
                        destroy_debug_window((DebugWindowType)i);
                        was_debug = true;
                        break;
                    }
                }
                if (was_debug) break;

                // Main window closed -> quit
                if (windowID == SDL_GetWindowID(this->main_window)){
                    for (int i = 0; i < NUM_DEBUG_WINDOWS; i++)
                        destroy_debug_window((DebugWindowType)i);
                    SDL_DestroyWindow(this->main_window);
                    SDL_DestroyRenderer(this->main_renderer);
                    SDL_DestroyTexture(this->main_texture);
                    return false;
                }
                break;
            }
            case SDL_EVENT_KEY_DOWN:
                switch (event.key.scancode){
                    case SDL_SCANCODE_ESCAPE:
                        this->dbg->dbg_level = FULL_DBG;
                        break;
                    default:
                        if (!event.key.repeat)
                            this->controller.enqueue_event(event.key.scancode, Controller_event_type::KEY_DOWN);
                        break;
                }
                break;
            case SDL_EVENT_KEY_UP:
                if (!event.key.repeat)
                    this->controller.enqueue_event(event.key.scancode, Controller_event_type::KEY_UP);
                break;
            default:
                break;
        }
    }
    return true;
}

// --- Update ---

bool Ui::update() {
    // Process pending debug window toggles
    for (int i = 0; i < NUM_DEBUG_WINDOWS; i++){
        if (debug_toggle_requested[i]){
            debug_toggle_requested[i] = false;
            if (debug_windows[i].active)
                destroy_debug_window((DebugWindowType)i);
            else
                create_debug_window((DebugWindowType)i);
        }
    }

    if(!this->handle_events())
        return false;

    this->mem.copy_mem(this->mem_copy);
    this->main_screen_update();
    this->tiles_dbg_update();
    this->bg_map_dbg_update();
    this->win_map_dbg_update();
    this->oam_dbg_update();
    return true;
}

// --- Main screen ---
void Ui::clear_main_screen(){
    std::scoped_lock<std::mutex> lock(video_buffer_mutex);
    std::fill_n(this->video_buffer, XRES * YRES, 0xFF000000); // Clear to black
    SDL_UpdateTexture(this->main_texture, NULL, this->video_buffer, XRES * sizeof(u32));
    SDL_RenderClear(this->main_renderer);
    SDL_RenderTexture(this->main_renderer, this->main_texture, NULL, NULL);
    SDL_RenderPresent(this->main_renderer);
}
void Ui::write_pixel(int x, int y, u32 color){
    if (x < 0 || x >= XRES || y < 0 || y >= YRES) return;
    std::scoped_lock<std::mutex> lock(video_buffer_mutex);
    this->video_buffer[y * XRES + x] = color;
}
void Ui::main_screen_update(){
    video_buffer_mutex.lock();
    SDL_UpdateTexture(this->main_texture, NULL, this->video_buffer, XRES * sizeof(u32));
    video_buffer_mutex.unlock();
    SDL_RenderClear(this->main_renderer);
    SDL_RenderTexture(this->main_renderer, this->main_texture, NULL, NULL);
    SDL_RenderPresent(this->main_renderer);
}

// --- Debug window update stubs (rendering logic will be added later) ---

void Ui::draw_tile(u32* pixel_buf, int buf_w, u16 tile_addr, int x, int y, u8 palette){
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

void Ui::tiles_dbg_update(){
    DebugWindow& dw = debug_windows[(int)DebugWindowType::TILES];
    if (!dw.active) return;

    u32 pixel_buf[TILE_DBG_W * TILE_DBG_H];
    std::fill_n(pixel_buf, TILE_DBG_W * TILE_DBG_H, GRID_COLOR);

    for (int tile_idx = 0; tile_idx < 384; tile_idx++){
        u16 tile_addr = 0x8000 + tile_idx * 16;
        int x = (tile_idx % 16) * 9;
        int y = (tile_idx / 16) * 9;
        draw_tile(pixel_buf, TILE_DBG_W, tile_addr, x, y, 0b11100100);
    }

    SDL_UpdateTexture(dw.texture, NULL, pixel_buf, TILE_DBG_W * sizeof(u32));
    SDL_RenderClear(dw.renderer);
    SDL_RenderTexture(dw.renderer, dw.texture, NULL, NULL);
    SDL_RenderPresent(dw.renderer);
}
void Ui::bg_map_dbg_update(){
    DebugWindow& dw = debug_windows[(int)DebugWindowType::BG_MAP];
    if (!dw.active) return;
    u32 pixel_buf[BG_MAP_DBG_W * BG_MAP_DBG_H];
    std::fill_n(pixel_buf, BG_MAP_DBG_W * BG_MAP_DBG_H, GRID_COLOR);
    u16 bg_map_base_addr = mem_copy[LCDC_ADDR] & 0x8 ? 0x9C00 : 0x9800;
    u16 tile_data_base = mem_copy[LCDC_ADDR] & 0x10 ? 0x8000 : 0x9000;
    for (int i=0; i < 1024; i++){
        u16 tile_idx = mem_copy[bg_map_base_addr + i];
        u16 tile_addr = tile_data_base;
        if(tile_data_base == 0x9000)
            tile_addr += (i8)tile_idx * 16; // Signed index
        else
            tile_addr += tile_idx * 16;      // Unsigned index
        int x = (i % 32) * 9;
        int y = (i / 32) * 9;
        draw_tile(pixel_buf, BG_MAP_DBG_W, tile_addr, x, y, mem_copy[BGP_ADDR]);
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

    SDL_UpdateTexture(dw.texture, NULL, pixel_buf, BG_MAP_DBG_W * sizeof(u32));
    SDL_RenderClear(dw.renderer);
    SDL_RenderTexture(dw.renderer, dw.texture, NULL, NULL);
    SDL_RenderPresent(dw.renderer);
}
void Ui::win_map_dbg_update(){
    DebugWindow& dw = debug_windows[(int)DebugWindowType::WIN_MAP];
    if (!dw.active) return;
    u32 pixel_buf[WIN_MAP_DBG_W * WIN_MAP_DBG_H];
    std::fill_n(pixel_buf, WIN_MAP_DBG_W * WIN_MAP_DBG_H, GRID_COLOR);
    u16 win_map_base_addr = mem_copy[LCDC_ADDR] & 0x40 ? 0x9C00 : 0x9800;
    u16 tile_data_base = mem_copy[LCDC_ADDR] & 0x10 ? 0x8000 : 0x9000;
    for (int i=0; i < 1024; i++){
        u16 tile_idx = mem_copy[win_map_base_addr + i];
        u16 tile_addr = tile_data_base;
        if(tile_data_base == 0x9000)
            tile_addr += (i8)tile_idx * 16; // Signed index
        else
            tile_addr += tile_idx * 16;      // Unsigned index
        int x = (i % 32) * 9;
        int y = (i / 32) * 9;
        draw_tile(pixel_buf, WIN_MAP_DBG_W, tile_addr, x, y, mem_copy[BGP_ADDR]);
    }
    SDL_UpdateTexture(dw.texture, NULL, pixel_buf, WIN_MAP_DBG_W * sizeof(u32));
    SDL_RenderClear(dw.renderer);
    SDL_RenderTexture(dw.renderer, dw.texture, NULL, NULL);
    SDL_RenderPresent(dw.renderer);
}
void Ui::oam_dbg_update(){
    DebugWindow& dw = debug_windows[(int)DebugWindowType::OAM];
    if (!dw.active) return;

    u32 pixel_buf[OAM_DBG_W * OAM_DBG_H];
    std::fill_n(pixel_buf, OAM_DBG_W * OAM_DBG_H, GRID_COLOR);

    bool tall     = mem_copy[LCDC_ADDR] & 0x04;
    int sprite_h  = tall ? 16 : 8;

    for (int i = 0; i < 40; i++){
        Sprite spr(&mem_copy[0xFE00 + i * 4]);

        u8 tile_index = spr.tile_index;
        if (tall) tile_index &= 0xFE;
        u16 tile_addr = 0x8000 + tile_index * 16;
        u8 palette    = mem_copy[spr.palette ? OBP1_ADDR : OBP0_ADDR];

        int px = (i % 10) * 9;
        int py = (i / 10) * 17;

        for (int r = 0; r < sprite_h; r++){
            int src_row  = spr.y_flip ? (sprite_h - 1 - r) : r;
            u16 row_addr = tile_addr + src_row * 2;
            u8 lo = mem_copy[row_addr];
            u8 hi = mem_copy[row_addr + 1];
            for (int c = 0; c < 8; c++){
                int bit      = spr.x_flip ? c : (7 - c);
                int color_id = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);
                int mapped   = (palette >> (color_id * 2)) & 0x3;
                pixel_buf[(py + r) * OAM_DBG_W + (px + c)] = gb_palette[mapped];
            }
        }
    }

    SDL_UpdateTexture(dw.texture, NULL, pixel_buf, OAM_DBG_W * sizeof(u32));
    SDL_RenderClear(dw.renderer);
    SDL_RenderTexture(dw.renderer, dw.texture, NULL, NULL);
    SDL_RenderPresent(dw.renderer);
}

Ui_ss Ui::save_state(){
    Ui_ss state;
    {
        std::scoped_lock<std::mutex> lock(video_buffer_mutex);
        std::copy(std::begin(this->video_buffer), std::end(this->video_buffer), std::begin(state.video_buffer));
    }
    return state;
}
void Ui::load_state(const Ui_ss& state){
    std::scoped_lock<std::mutex> lock(video_buffer_mutex);
    std::copy(std::begin(state.video_buffer), std::end(state.video_buffer), std::begin(this->video_buffer));
    SDL_UpdateTexture(this->main_texture, NULL, this->video_buffer, XRES * sizeof(u32));
    SDL_RenderClear(this->main_renderer);
    SDL_RenderTexture(this->main_renderer, this->main_texture, NULL, NULL);
    SDL_RenderPresent(this->main_renderer);
}