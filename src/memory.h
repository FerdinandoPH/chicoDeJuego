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
enum class MBC_type{NONE, MBC1, MBC2, MBC3, MBC5, MBC6, MBC7, MMM01, CAMERA, TAMA5, HuC1, HuC3};
enum class MBC_action{INIT, READ, WRITE};
enum class MBC_ret_type{DISCARD, KEEP, RETURN};
typedef struct{
    bool has_battery=false;
    bool has_rtc=false;
    bool has_rumble=false;
    bool has_sensor=false;
} Cart_features;
typedef struct{
    MBC_ret_type type;
    u8 data;
} MBC_result;
extern std::unordered_map<u8,MBC_type> cart_type_to_mbc;
extern std::unordered_set<u8> cart_with_battery;
extern std::unordered_set<u8> cart_with_timer;
extern std::unordered_set<u8> cart_with_rumble;

extern std::unordered_map<u8,size_t> ram_size_to_bytes;
extern std::unordered_map<u8,size_t> rom_size_to_number_of_banks;
extern std::unordered_map<MBC_type, std::string> mbc_names;
extern std::unordered_map<MBC_type, void (Memory::*)(MBC_action, u16, u8, MBC_result*)> mbc_handlers;

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

struct MBC_state{
    virtual ~MBC_state() = default;
};
struct MBC1_state: public MBC_state{
    bool ext_ram_enabled = false;
    bool advanced_banking_mode = false;
    size_t rom_banks = 0;
    size_t ram_banks = 0;
    u8 reg_2000_3FFF = 1;
    u8 reg_2000_3FFF_mask= 0b00011111;
    u8 reg_4000_5FFF = 0;
};
struct MBC3_state: public MBC_state{
    bool ext_ram_enabled = false;
    size_t rom_banks = 0;
    size_t ram_banks = 0;
};
struct MBC5_state: public MBC_state{
    bool ext_ram_enabled = false;
    size_t rom_banks = 0;
    size_t ram_banks = 0;
    u8 reg_2000_2FFF = 0;
    u8 reg_3000_3FFF = 0;
    u8 reg_4000_5FFF = 0;
};
class Memory {

    private:
        MBC_type mbc_type;
        std::mutex mem_mutex;
        u8 _mem[0x10000];
        std::string _rom_filename;
        u8* _rom = nullptr;
        u8* _ram = nullptr;
        MBC_state* mbc_state = nullptr;
        size_t _ram_size = 0;
        size_t _ram_bank_size = 8192;
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
        void load_save();
        MBC_result process_MBC_read(u16 address);
        MBC_result process_MBC_write(u16 address, u8 data);
        size_t current_ram_bank = 0;
        size_t current_rom0_bank = 0;
        size_t current_rom1_bank = 1;
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
        Cart_features cart_features;
        bool is_protected = true;
        Memory();
        ~Memory();
        void dump();
        void copy_mem(u8* ptr);
        void reset();
        void set_dma(Dma* dma);
        void set_controller(Controller* controller);
        void set_vram_lock(bool locked);
        void set_oam_lock(bool locked);
        u8 readX(u16 address);
        void writeX(u16 address, u8 data);
        void writeX(u16 address, u16 data);
        u8 read(u16 address, bool from_cpu);
        void write(u16 address, u8 data, bool from_cpu);
        Proxy operator[](u16 address) { return Proxy(*this, address); }
        bool load_rom(const char* filename);
        std::string get_sha256();
        Cart_header get_cart_header();
        void save_ram();
        #pragma region MBC_handlers
            void change_banks(size_t new_rom0_bank, size_t new_rom1_bank, size_t new_ram_bank);
            void MBC1_handler(MBC_action action, u16 address, u8 data, MBC_result* result);
            void MBC1_change_banks();
            void MBC2_handler(MBC_action action, u16 address, u8 data, MBC_result* result);
            void MBC3_handler(MBC_action action, u16 address, u8 data, MBC_result* result);
            void MBC5_handler(MBC_action action, u16 address, u8 data, MBC_result* result);
            void MBC5_change_banks();
        #pragma endregion
};