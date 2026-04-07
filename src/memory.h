#pragma once
#include <cstdint>
#include "utils.h"
#include <mutex>
#include "dma.h"
#include <unordered_set>
#include <unordered_map>
#include <string>
#include "controller.h"


//#define SERIAL_LOG
enum class MBC_type{NONE = 0, MBC1 = 1, MBC2 = 2, MBC3 = 3, MBC5 = 5, MBC6 = 6, MBC7 = 7, MMM01 = 0xB, HuC1 = 0xC, HuC3 = 0xD};
extern std::unordered_map<MBC_type, std::string> mbc_names;

typedef struct{
    u8 entry[4];
    u8 nintendo_logo[0x30];
    char title[16];
    u16 license_code_new;
    u8 sgb_flag;
    u8 cart_type;
    u8 rom_size;
    u8 ram_size;
    u8 region_code;
    u16 license_code;
    u8 version;
    u8 header_checksum;
    u16 global_checksum;
} Cart_header;
typedef struct{
    bool change;
    u8 data;
} MBC_result;
class Memory {

    private:

        MBC_type mbc_type;
        std::mutex mem_mutex;
        u8 _mem[0x10000];
        u8* _rom = nullptr;
        size_t _rom_size = 0;
        std::unordered_set<u16> write_zero = {DIV_ADDR};
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
        MBC_result process_MBC_write(u16 address, u8 data);
        //u8 process_mbc_read(u16 address);
        Dma* dma;
        Controller* controller;
        bool vram_locked = false;
        bool oam_locked = false;
        #ifdef SERIAL_LOG
        FILE* serial_log;
        #endif
    public:
        Cart_header* rom_header;
        bool is_protected = true;
        Memory();
        ~Memory();
        u8 readX(u16 address);
        void writeX(u16 address, u8 data);
        void writeX(u16 address, u16 data);
        u8 read(u16 address, bool from_cpu);
        void write(u16 address, u8 data, bool from_cpu);
        Proxy operator[](u16 address) { return Proxy(*this, address); }
        bool load_rom(const char* filename);
        std::string get_sha256();
        void dump();
        void reset();
        void set_dma(Dma* dma);
        void set_controller(Controller* controller);
        void set_vram_lock(bool locked);
        void set_oam_lock(bool locked);
        Cart_header get_cart_header();

};