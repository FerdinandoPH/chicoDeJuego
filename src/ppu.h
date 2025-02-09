#pragma once
#include <utils.h>
#include <memory.h>
#include <map>
#define LY_ADDR 0xFF44
#define LCD_STAT_ADDR 0xFF41
#define LYC_ADDR 0xFF45
enum class Ppu_mode{OAM, TRANSFER, VBLANK, HBLANK};
extern std::map<Ppu_mode, std::string> ppu_mode_names;
class Ppu{
    private:
        Memory& mem;
        int scale;
        Ppu_mode ppu_mode;
        int line_ticks = 0;
        void inc_ly();
        void change_mode(Ppu_mode mode);

    public:
        Ppu(Memory& mem, int scale = 4);
        void tick();
        std::string toString();
};