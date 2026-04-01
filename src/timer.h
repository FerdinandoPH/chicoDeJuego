#pragma once

#include <utils.h>
#include <cpu.h>
#include<memory.h>
typedef struct{
    bool enabled;
    u8 clock_select;
}Tac_Struct;
class Timer{
    private:
        int internal_clock = 0;
    public:
        u16 clock_table [4] = {256*4, 4*4, 16*4, 64*4}; //All 4 TIMA speeds
        u16 tima_accumulation = 0;
        Tac_Struct get_tac();
        Cpu& cpu;
        Memory& mem;
        Timer(Cpu& cpu, Memory& mem);
        bool trigger_on_next = false;
        void tick();
        void reset();
        std::string tac_toString();
        std::string toString();
};