#pragma once
#include <utils.h>
#include <memory.h>
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
#define XRES 160
#define YRES 144
enum class Ppu_mode{OAM, TRANSFER, VBLANK, HBLANK};
extern std::unordered_map<Ppu_mode, std::string> ppu_mode_names;

enum class Pixel_fetcher_state{READ_TILE, READ_DATA_LO, READ_DATA_HI, PUSH};
enum class Tile_type{BG, WINDOW, SPRITE};
enum class Palette{BG, OBJ0, OBJ1};
class Pixel_FIFO;
typedef struct{
    u8 color_index;
    Palette palette;
    Tile_type tile_type;
}Pixel;
class Pixel_Fetcher{
    private:
        Memory& mem;
        Sprite (&line_oam)[10]; // Deprecated
        int& sprites_in_line; // Deprecated
        Sprite spr;
        int spr_line;
        Pixel_FIFO* fifo;
        u16 tile_addr;
        u8 dx;
        u8 dy;
        u8 win_dx;
        u8 win_dy;
        u8 tile_lo;
        u8 tile_hi;
        Pixel pixels[8];
        Pixel_fetcher_state state;
        Tile_type tile_type;
        Tile_type tile_type_bak;
        u8 get_line_to_read();
        void assemble_pixels();
    public:
        Pixel_Fetcher(Memory& mem, Sprite (&line_oam)[10], int& sprites_in_line);
        void set_fifo(Pixel_FIFO* fifo);
        void tick();
        void fetch_sprite(Sprite spr, int line);
        void change_general_state(Pixel_fetcher_state state);
        void new_line();
        void new_frame();
};
class Pixel_FIFO{
    private:
        Memory& mem;
        u8 dx;
        Pixel_Fetcher* fetcher;
        Sprite (&line_oam)[10];
        int& sprites_in_line;
        std::deque<Pixel> fifo;
    public:
        Pixel_FIFO(Memory& mem, Sprite (&line_oam)[10], int& sprites_in_line, Pixel_Fetcher* fetcher);
        u8 get_x();
        void push(Pixel* pixels);
        void mix_sprite(Pixel* pixels);
        void tick();
        int size();
        void clear();
};
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
class Ppu{
    private:
        Memory& mem;
        Pixel_Fetcher* fetcher;
        Pixel_FIFO* fifo;
        int scale;
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
        Ppu(Memory& mem, int scale = 4);
        void tick();
        std::string toString();
};