#pragma once
#include "utils.h"
#include "screen_specs.h"
#include "memory.h"
#include "host.h"
#include <unordered_map>
#include <deque>


enum class Ppu_mode{OAM, TRANSFER, VBLANK, HBLANK};
extern std::unordered_map<Ppu_mode, std::string> ppu_mode_names;
enum class Pixel_fetcher_state{READ_TILE, READ_TILE_2, READ_DATA_LO, READ_DATA_LO_2, READ_DATA_HI, READ_DATA_HI_2, PUSH};
enum class Tile_type{BG, WINDOW, SPRITE};
enum class DMG_Palette{BG, OBJ0, OBJ1};
class Pixel_FIFO;
struct Sprite{
    int oam_idx = 0;
    u8 y_pos;
    u8 x_pos;
    u8 tile_index;
    bool priority;
    bool y_flip;
    bool x_flip;
    u8 dmg_palette;
    //The rest is CGB exclusive, not implemented yet
    u8 cgb_bank;
    u8 cgb_palette_index;
    Sprite() = default;
    Sprite(int idx, u8 data[4]){
        this->oam_idx = idx;
        this->y_pos = data[0];
        this->x_pos = data[1];
        this->tile_index = data[2];
        this->priority = (data[3] & 0x80);
        this->y_flip = (data[3] & 0x40);
        this->x_flip = (data[3] & 0x20);
        this->dmg_palette = (data[3] & 0x10);
        this->cgb_bank = (data[3] & 0x8) >> 3;
        this->cgb_palette_index = (data[3] & 0x7);
    };
};
typedef struct{
    u8 color_index;
    DMG_Palette dmg_palette;
    u8 cgb_palette_index;
    Tile_type tile_type;

    int spr_idx;
    bool spr_priority;

    bool cgb_bgwin_priority;
}Pixel;
struct Tile_bgwin_attrs{
    bool priority;
    bool y_flip;
    bool x_flip;
    u8 cgb_bank;
    u8 cgb_palette_index;
};
struct Tile{
    Tile_type type;
    u16 addr;
    u8 cgb_bank;
    Tile_bgwin_attrs attrs;
    u8 tile_lo;
    u8 tile_hi;
};
typedef struct{
    Sprite spr;
    int spr_line;
    Tile current_tile;
    u8 f_scx;
    u8 f_scy;
    u8 f_lx;
    u8 f_ly;
    u8 f_win_lx;
    u8 f_win_ly;
    Pixel_fetcher_state state;
    Tile_type tile_type;
    Tile_type tile_type_bak;
    Pixel fetcher_pixel_buffer[8];
}Pixel_Fetcher_ss;
class Pixel_Fetcher{
    private:
        Memory& mem;
        Sprite spr;
        GB_model& gb_model;
        int spr_line;
        Pixel_FIFO* fifo;
        Tile current_tile;
        // u16 tile_addr;
        u8 f_scx;
        u8 f_scy;
        u8 f_lx;
        u8 f_ly;
        u8 f_win_lx;
        u8 f_win_ly;
        Pixel_fetcher_state state;
        Tile_type tile_type;
        Tile_type tile_type_bak;
        // u8 tile_lo;
        // u8 tile_hi;
        Pixel fetcher_pixel_buffer[8];
        u8 get_line_to_read();
        void assemble_pixels();
        Tile_bgwin_attrs get_tile_attrs(u16 tile_addr);
    public:
        Pixel_Fetcher(Memory& mem, GB_model& gb_model);
        void set_f_win_ly(u8 new_value);
        void set_fifo(Pixel_FIFO* fifo);
        void tick();
        void fetch_sprite(Sprite spr);
        void change_to_win();
        void new_line();
        void new_frame();
        Pixel_Fetcher_ss save_state();
        void load_state(const Pixel_Fetcher_ss& state);
};
typedef struct{
    u8 lx, ly, triggered_wx, win_ly, pixels_to_discard;
    bool increase_win_ly, waiting_for_sprite, wx_cond, wy_cond, window_active, sprites_in_pixel_done;
    std::deque<Pixel> pixels;
    std::deque<Pixel> obj_pixels;
    std::deque<Sprite> sprites_in_pixel;
}Pixel_FIFO_ss;
class Pixel_FIFO{
    private:
        Memory& mem;
        Host* host;
        GB_model& gb_model;
        Sprite (&line_oam)[10];
        int& sprites_in_line;
        bool debug_me = false;
        Pixel_Fetcher* fetcher;
        u8 lx;
        u8 ly;
        u8 triggered_wx;
        u8 win_ly;
        bool increase_win_ly = false;
        bool waiting_for_sprite = false;
        bool wx_cond = false;
        bool wy_cond = false;
        bool window_active = false;
        bool sprites_in_pixel_done = false;
        u8 pixels_to_discard = 0;
        std::deque<Pixel> pixels = std::deque<Pixel>();
        std::deque<Pixel> obj_pixels = std::deque<Pixel>();
        std::deque<Sprite> sprites_in_pixel = std::deque<Sprite>();
        bool determine_cgb_obj_priority(Pixel obj_pixel, Pixel bgwin_pixel);
        u32 get_final_color(Pixel pixel);
        u32 color_cgb_to_rgb(u16 cgb_color);
    public:
        Pixel_FIFO(Memory& mem, Host* host, GB_model& gb_model, Sprite (&line_oam)[10], int& sprites_in_line, Pixel_Fetcher* fetcher);
        void new_line();
        void new_frame();
        u8 get_lx();
        u8 get_triggered_wx();
        u8 get_win_ly();
        void push(Pixel* pixels);
        void push_obj(Pixel* pixels);
        void tick();
        bool is_main_fifo_empty();
        Pixel_FIFO_ss save_state();
        void load_state(const Pixel_FIFO_ss& state);
};

typedef struct{
    u8 lcdc, stat, scy, scx, ly, lyc;
}Ppu_trace;
typedef struct{
    Pixel_Fetcher_ss fetcher;
    Pixel_FIFO_ss fifo;
    Sprite oam[40];
    Sprite line_oam[10];
    int sprites_in_line;
    bool is_on, startup;
    Ppu_mode ppu_mode;
    int line_ticks;

}Ppu_ss;
class Ppu{
    private:
        Memory& mem;
        Host* host;
        GB_model& gb_model;
        Pixel_Fetcher* fetcher;
        Pixel_FIFO* fifo;
        bool enabled = true;
        Ppu_mode ppu_mode;
        int line_ticks = 0;
        bool startup = true;
        void inc_ly();
        void change_mode(Ppu_mode mode);
        void load_oam();
        void load_line_oam();
        void check_lyc_at_restart();
    public:
        bool vblank_triggered = false;
        Sprite oam[40];
        Sprite line_oam[10];
        int sprites_in_line;
        Ppu(Memory& mem, Host* host, GB_model& gb_model);
        Ppu_mode get_mode(){return ppu_mode;};
        void tick();
        void reset();
        std::string toString();
        Ppu_trace get_trace();
        Ppu_ss save_state();
        void load_state(const Ppu_ss& state);
        bool is_enabled() { return this->enabled; }
};