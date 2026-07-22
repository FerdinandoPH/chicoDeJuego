#pragma once
#include "utils.h"
#include "hw_reg_def.h"
class Memory;
class Ppu;
class Cpu;

typedef struct{
    u16 source;
    u16 dest;
    u16 bytes_left;
    bool transferring;
}Dma_ss;
class Dma{
    private:
        Memory* mem;
        u16 source;
        u16 dest;
        u16 bytes_left;
    public:
        bool transferring = false;
        Dma(Memory* mem);
        void start(u16 source, u16 dest, u16 length);
        void tick();
        std::string toString();
        Dma_ss save_state();
        void load_state(const Dma_ss& state);
};

enum class Vdma_mode{GENERAL, HBLANK};
enum class Vdma_state{IDLE, TRANSFERRING, WAITING_FOR_HBLANK, WAITING_FOR_NEXT_LINE};
class Vdma{
    private:
        Memory* mem;
        Ppu* ppu;
        Cpu* cpu;
        u16 source;
        u16 dest;
        u16 bytes_left;
        u16 bytes_transferred_in_round = 0;
        Vdma_mode mode;
    public:
        Vdma_state state = Vdma_state::IDLE;
        Vdma(Memory* mem, Ppu* ppu, Cpu* cpu);
        Vdma_mode get_mode(){return mode;};
        void set_src(u16 addr, u8 data);
        void set_dest(u16 addr, u8 data);
        u8 set_VDMA5(u8 data);

        void tick();
        std::string toString();
};