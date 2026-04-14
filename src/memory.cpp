#include "memory.h"
#include "utils.h"
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <array>
#include <sstream>
#include <iomanip>
#include <vector>

std::unordered_map<MBC_type, std::string> mbc_names = {
    {MBC_type::NONE, "None"},
    {MBC_type::MBC1, "MBC1"},
    {MBC_type::MBC2, "MBC2"},
    {MBC_type::MBC3, "MBC3"},
    {MBC_type::MBC5, "MBC5"},
    {MBC_type::MBC6, "MBC6"},
    {MBC_type::MBC7, "MBC7"},
    {MBC_type::MMM01, "MMM01"},
    {MBC_type::HuC1, "HuC1"},
    {MBC_type::HuC3, "HuC3"}
};
Memory::Memory() : mem_mutex() {
    memset(_mem, 0, sizeof(_mem));
    this->reset();
}
Memory::~Memory() {
    if (_rom) {
        free(_rom);
    }
    if(_ram){
        free(_ram);
    }
    #ifdef SERIAL_LOG
    if (serial_log) {
        fclose(serial_log);
    }
    #endif
}
/*When reading/writing to memory from CPU, sometimes some side effects will occur
 *readX and writeX will ignore these side effects*/
void Memory::write(u16 address, u8 data, bool from_cpu) {
    std::scoped_lock<std::mutex> lock(this->mem_mutex);
    bool writable = true;
    from_cpu = from_cpu && this->is_protected;
    if (from_cpu){
        MBC_result mbc_result = process_MBC_write(address, data);
        if(mbc_result.type == MBC_ret_type::RETURN){
            return;
        }
        if(address<0x8000){
            //std::cout<<"Rom write attempt at address: "<<numToHexString(address, 4)<<" and value: "<<numToHexString(data, 2)<<std::endl;
            //writable = false;
            return;
        }
        if(dma->transferring && !BETWEEN(address, 0xFF80, 0xFFFE)){
            std::cout<<"Writing during DMA transfer at address: "<<numToHexString(address, 4)<<" and value: "<<numToHexString(data, 2)<<std::endl;
            //writable = false;
            return;
        }
        if(vram_locked && BETWEEN(address, 0x8000, 0x9FFF)){
            std::cout<<"Writing to VRAM while locked at address: "<<numToHexString(address, 4)<<" and value: "<<numToHexString(data, 2)<<std::endl;
            //writable = false;
            return;
        }
        if(oam_locked && BETWEEN(address, 0xFE00, 0xFE9F)){
            std::cout<<"Writing to OAM while locked at address: "<<numToHexString(address, 4)<<" and value: "<<numToHexString(data, 2)<<std::endl;
            //writable = false;
            return;
        }
        if (write_zero.find(address) != write_zero.end()){
            data = 0;
        }
        else if (address == DMA_ADDR){
            if (data > 0xDF){
                std::cout<<"Invalid DMA source address: "<<numToHexString(data, 2)<<std::endl;
                data = 0xDF;
            }
            this->dma->start(static_cast<u16>(data)*0x100, 0xFE00, 0xA0);
        }
        switch(address){
            case JOYP_ADDR:
                data = controller->joyp_change(data);
                break;
            case TAC_ADDR:
                data |= 0xF8;
                break;
            case IF_ADDR:
                data |= 0xE0;
                break;
        }
        #ifdef SERIAL_LOG
        if(address == 0xFF02 && data == 0x81){
            fprintf(serial_log, "%c", this->_mem[0xFF01]);
        }
        #endif
    }
    if(writable)
        _mem[address] = data;
}

u8 Memory::read(u16 address, bool from_cpu) {
    std::scoped_lock<std::mutex> lock(this->mem_mutex);
    u8 data = _mem[address];
    from_cpu = from_cpu && this->is_protected;
    if(from_cpu){
        MBC_result mbc_result = process_MBC_read(address);
        if(mbc_result.type == MBC_ret_type::RETURN){
            return mbc_result.data;
        }
        if(mbc_result.type == MBC_ret_type::KEEP){
            data = mbc_result.data;
        }
        if(dma->transferring && !BETWEEN(address, 0xFF80, 0xFFFE)){ //Only HRAM should be accesible
            std::cout<<"Reading during DMA transfer at address: "<<numToHexString(address, 4)<<std::endl;
            return 0xFF;
        }
        if(vram_locked && BETWEEN(address, 0x8000, 0x9FFF)){
            std::cout<<"Reading from VRAM while locked at address: "<<numToHexString(address, 4)<<std::endl;
            return 0xFF;
        }
        if(oam_locked && BETWEEN(address, 0xFE00, 0xFE9F)){
            std::cout<<"Reading from OAM while locked at address: "<<numToHexString(address, 4)<<std::endl;
            return 0xFF;
        }
        if(_ram_size <=0 && BETWEEN(address, 0xA000, 0xBFFF)){
            std::cout<<"Reading from non-existent RAM at address: "<<numToHexString(address, 4)<<std::endl;
            return 0xFF;
        }
    }
    return data;
}

u8 Memory::readX(u16 address) { 
    return this->read(address, false);
}

void Memory::writeX(u16 address, u8 data) {
    this->write(address, data, false);
}
void Memory::writeX(u16 address, u16 data) {
    this->write(address, static_cast<u8>(data & 0xFF), false);
}




void Memory::dump() { //Writes the current state of memory into a file and opens it with a HEX editor
    std::scoped_lock<std::mutex> lock(this->mem_mutex);
    FILE* file = fopen("mem.hexd", "wb");
    if (file) {
        size_t writtenData = fwrite(_mem, 1, 0x10000, file);
        if (writtenData != 0x10000) {
            printf("Error writing mem.hexd\n");
        }
        fclose(file);
        #ifdef _WIN32
            createProcess("mem.hexd");
        #endif
    }
    else {
        printf("Error writing mem.hexd\n");
    }
}

void Memory::reset(){
    std::scoped_lock<std::mutex> lock(this->mem_mutex);
    memset(_mem, 0, sizeof(_mem));
    if (_rom != NULL)
        memcpy(_mem, _rom, 0x8000);
    this->load_initial_values();
    #ifdef SERIAL_LOG
    serial_log = fopen("serial_log.txt", "w");
    #endif
}

void Memory::load_initial_values(){
    // Power-up values for DMG/MGB from Pan Docs (PC = 0x0100)
    _mem[JOYP_ADDR] = 0xCF;
    _mem[SB_ADDR] = 0x00;
    _mem[SC_ADDR] = 0x7E;
    _mem[DIV_ADDR] = 0xAB;
    _mem[TIMA_ADDR] = 0x00;
    _mem[TMA_ADDR] = 0x00;
    _mem[TAC_ADDR] = 0xF8;
    _mem[IF_ADDR] = 0xE1;

    _mem[NR10_ADDR] = 0x80;
    _mem[NR11_ADDR] = 0xBF;
    _mem[NR12_ADDR] = 0xF3;
    _mem[NR13_ADDR] = 0xFF;
    _mem[NR14_ADDR] = 0xBF;
    _mem[NR21_ADDR] = 0x3F;
    _mem[NR22_ADDR] = 0x00;
    _mem[NR23_ADDR] = 0xFF;
    _mem[NR24_ADDR] = 0xBF;
    _mem[NR30_ADDR] = 0x7F;
    _mem[NR31_ADDR] = 0xFF;
    _mem[NR32_ADDR] = 0x9F;
    _mem[NR33_ADDR] = 0xFF;
    _mem[NR34_ADDR] = 0xBF;
    _mem[NR41_ADDR] = 0xFF;
    _mem[NR42_ADDR] = 0x00;
    _mem[NR43_ADDR] = 0x00;
    _mem[NR44_ADDR] = 0xBF;
    _mem[NR50_ADDR] = 0x77;
    _mem[NR51_ADDR] = 0xF3;
    _mem[NR52_ADDR] = 0xF1;

    _mem[LCDC_ADDR] = 0x91;
    _mem[LCD_STAT_ADDR] = 0x85;
    _mem[SCY_ADDR] = 0x00;
    _mem[SCX_ADDR] = 0x00;
    _mem[LY_ADDR] = 0x00;
    _mem[LYC_ADDR] = 0x00;
    _mem[DMA_ADDR] = 0xFF;
    _mem[BGP_ADDR] = 0xFC;
    // OBP0/OBP1 are unspecified on DMG/MGB at hand-off; leave current memory value.
    _mem[WY_ADDR] = 0x00;
    _mem[WX_ADDR] = 0x00;
    _mem[IE_ADDR] = 0x00;

    current_ram_bank = 0;
    current_rom0_bank = 0;
    current_rom1_bank = 1;
}
void Memory::set_dma(Dma* dma){
    this->dma = dma;
}
void Memory::set_controller(Controller* controller){
    this->controller = controller;
}
void Memory::set_vram_lock(bool locked){
    this->vram_locked = locked;
}
void Memory::set_oam_lock(bool locked){
    this->oam_locked = locked;
}
