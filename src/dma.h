#pragma once
#include "utils.h"

class Memory;

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