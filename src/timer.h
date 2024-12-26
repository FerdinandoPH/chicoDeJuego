#pragma once

#include <utils.h>
#include <cpu.h>
#include<memory.h>
typedef struct{
    bool enabled;
    u8 clock_select;
}Tac_Struct;
class Timer{
    public:
        u16 clock_table [4] = {256, 4, 16, 64};
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