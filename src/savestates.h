#pragma once
#include "utils.h"
#include "cpu.h"
#include "dma.h"
#include "memory.h"
#include "ppu.h"
#include "timer.h"
#include "ui.h"
struct Save_state{
    Cpu_ss cpu_state;
    Timer_ss timer_state;
    Ppu_ss ppu_state;
    Memory_ss mem_state;
    Dma_ss dma_state;
    Ui_ss ui_state;
    int ticks;
    int ticks_since_last_sync;
    std::string sha256;
};

class SaveStateManager{
    private:
        Cpu* cpu;
        Timer* timer;
        Ppu* ppu;
        Memory* mem;
        Dma* dma;
        Ui* ui;
        int& ticks;
        int& ticks_since_last_sync;
        std::string filename;
    public:
        SaveStateManager(Cpu* cpu, Timer* timer, Ppu* ppu, Memory* mem, Dma* dma, Ui* ui, int& ticks, int& ticks_since_last_sync);
        void save_state();
        void load_state();
        void set_filename(const std::string& new_filename) { filename = new_filename; }
};