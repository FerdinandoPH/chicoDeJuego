#pragma once
#include <utils.h>

class Memory;


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
};