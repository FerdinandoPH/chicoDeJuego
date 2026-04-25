#pragma once

#include "utils.h"
#include "cpu.h"
#include "memory.h"
#define DIV_ADDR 0xFF04
#define TIMA_ADDR 0xFF05
#define TMA_ADDR 0xFF06
#define TAC_ADDR 0xFF07
#define IF_ADDR 0xFF0F
typedef struct{
    bool enabled;
    u8 clock_select;
}Tac_Struct;
typedef struct{
    u8 div;
    u8 tima;
    u8 tma;
    u8 tac;
}Timer_trace;
typedef struct{
    u8 div, tima, tma, tac;
    u16 tima_accumulation;
    int internal_clock;
} Timer_ss;

class Timer{
    private:
        int internal_clock = 0;
    public:
        static const u16 clock_table [4];
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
        Timer_trace get_trace();
        Timer_ss save_state();
        void load_state(const Timer_ss& state);
};