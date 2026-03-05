#pragma once
#include <utils.h>
#include <screen_specs.h>
#include <memory.h>
#include <ui.h>
#include <unordered_map>
#include <deque>
#define LY_ADDR 0xFF44
#define LCD_STAT_ADDR 0xFF41
#define LCDC_ADDR 0xFF40
#define LYC_ADDR 0xFF45
#define SCX_ADDR 0xFF43
#define SCY_ADDR 0xFF42
#define WX_ADDR 0xFF4B
#define WY_ADDR 0xFF4A
#define BGP_ADDR 0xFF47
#define OBP0_ADDR 0xFF48
#define OBP1_ADDR 0xFF49

enum class Ppu_mode{OAM, TRANSFER, VBLANK, HBLANK};
extern std::unordered_map<Ppu_mode, std::string> ppu_mode_names;
enum class Pixel_fetcher_state{READ_TILE, READ_TILE_2, READ_DATA_LO, READ_DATA_LO_2, READ_DATA_HI, READ_DATA_HI_2, PUSH};
enum class Tile_type{BG, WINDOW, SPRITE};
enum class Palette{BG, OBJ0, OBJ1};
class Pixel_FIFO;
struct Sprite{
    u8 y_pos;
    u8 x_pos;
    u8 tile_index;
    bool priority;
    bool y_flip;
    bool x_flip;
    u8 palette;
    bool bank_1;
    //Other attributes are CGB exclusive
    Sprite() = default;
    Sprite(u8 data[4]){
        this->y_pos = data[0];
        this->x_pos = data[1];
        this->tile_index = data[2];
        this->priority = (data[3] & 0x80) != 0;
        this->y_flip = (data[3] & 0x40) != 0;
        this->x_flip = (data[3] & 0x20) != 0;
        this->palette = (data[3] & 0x10) != 0;
        this->bank_1 = (data[3] & 0x8) != 0;
    };
};
typedef struct{
    u8 color_index;
    Palette palette;
    Tile_type tile_type;
    bool spr_priority;
}Pixel;

class Pixel_Fetcher{
    private:
        Memory& mem;
        Sprite spr;
        int spr_line;
        Pixel_FIFO* fifo;
        u16 tile_addr;
        u8 f_scx;
        u8 f_fine_scx;
        u8 f_scy;
        u8 f_lx;
        u8 f_ly;
        u8 f_win_lx;
        u8 f_win_ly;
        Pixel_fetcher_state state;
        Tile_type tile_type;
        Tile_type tile_type_bak;
        u8 tile_lo;
        u8 tile_hi;
        Pixel pixel_buffer[8];
        u8 get_line_to_read();
        void assemble_pixels();
    public:
        Pixel_Fetcher(Memory& mem);
        void set_f_win_ly(u8 new_value);
        void set_fifo(Pixel_FIFO* fifo);
        void tick();
        void fetch_sprite(Sprite spr);
        void change_to_win();
        void new_line();
        void new_frame();
};
class Pixel_FIFO{
    private:
        Memory& mem;
        Ui* ui;
        Sprite (&line_oam)[10];
        int& sprites_in_line;
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
        u32 get_final_color(Pixel pixel);
    public:
        Pixel_FIFO(Memory& mem, Ui* ui, Sprite (&line_oam)[10], int& sprites_in_line, Pixel_Fetcher* fetcher);
        void new_line();
        void new_frame();
        u8 get_lx();
        u8 get_triggered_wx();
        u8 get_win_ly();
        void push(Pixel* pixels);
        void push_obj(Pixel* pixels);
        void tick();
        bool is_main_fifo_empty();
};

class Ppu{
    private:
        Memory& mem;
        Ui* ui;
        Pixel_Fetcher* fetcher;
        Pixel_FIFO* fifo;
        int scale;
        bool is_on = true;
        Ppu_mode ppu_mode;
        int line_ticks = 0;
        void inc_ly();
        void change_mode(Ppu_mode mode);
        void load_oam();
        void load_line_oam();
    public:
        Sprite oam[40];
        Sprite line_oam[10];
        int sprites_in_line;
        u32* video_buffer = new u32[XRES * YRES];
        Ppu(Memory& mem, Ui* ui, int scale = 4);
        void tick();
        std::string toString();
};