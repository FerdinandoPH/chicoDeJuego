#pragma once
#include <cstdint>
#include <utils.h>
#include <mutex>
#include <dma.h>
#include <unordered_set>
#define DMA_DIR 0xFF46
#define DIV 0xFF04
class Memory {

    private:
        typedef struct{
            u8 entry[4];
            u8 nintendo_logo[0x30];
            char title[16];
            u16 license_code;
            u16 license_code_new;
            u8 sgb_flag;
            u8 cart_type;
            u8 rom_size;
            u8 ram_size;
            u8 region_code;
            u8 version;
            u8 header_checksum;
            u16 global_checksum;
        } Cart_header;
        std::mutex mutex;
        u8 _mem[0x10000];
        u8* _rom = nullptr;
        std::unordered_set<u16> write_zero = {DIV};
        class Proxy{
            private:
                Memory& _memory;
                u16 _addr;
            public:
                Proxy(Memory& memory, u16 addr) : _memory(memory), _addr(addr) {}
                operator u8() const { return _memory.read(_addr, true); }
                Proxy& operator=(u8 data) { _memory.write(_addr, data, true); return *this; }
        };
        void load_initial_values();
        Dma* dma;
    public:
        Cart_header* rom_header;
        bool is_protected = false;
        Memory();
        ~Memory();
        u8 readX(u16 address);
        void writeX(u16 address, u8 data);
        void writeX(u16 address, u16 data);
        u8 read(u16 address, bool from_cpu);
        void write(u16 address, u8 data, bool from_cpu);
        Proxy operator[](u16 address) { return Proxy(*this, address); }
        bool load_rom(const char* filename);
        void dump();
        void reset();
        void set_dma(Dma* dma);


};