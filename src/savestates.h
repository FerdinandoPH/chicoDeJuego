#pragma once
#include "utils.h"
#include "cpu.h"
#include "dma.h"
#include "memory.h"
#include "ppu.h"
#include "timer.h"
#include "ui.h"
#include "apu.h"
// Bump whenever the binary layout of any *_ss struct changes, so older
// (now incompatible) save states are rejected instead of misread.
constexpr int SAVESTATE_VERSION = 2;

struct Save_state{
    int version;
    Cpu_ss cpu_state;
    Timer_ss timer_state;
    Ppu_ss ppu_state;
    Memory_ss mem_state;
    Dma_ss dma_state;
    Vdma_ss vdma_state;
    Ui_ss ui_state;
    Apu_ss apu_state;
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
        Vdma* vdma;
        Ui* ui;
        Apu* apu;
        int& ticks;
        int& ticks_since_last_sync;
        std::string filename;
    public:
        SaveStateManager(Cpu* cpu, Timer* timer, Ppu* ppu, Memory* mem, Dma* dma, Vdma* vdma, Ui* ui, Apu* apu, int& ticks, int& ticks_since_last_sync);
        void save_state();
        void load_state();
        void set_filename(const std::string& new_filename) { filename = new_filename; }
};