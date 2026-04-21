#include "memory.h"
#include <stdlib.h>
#include <cmath>
#include "sha256.h"
std::unordered_map<u8,size_t> ram_size_to_bytes = {
    {0, 0},{1, 0},{2, 8*1024},{3, 32*1024},{4, 128*1024},{5, 64*1024}
};
std::unordered_map<u8,size_t> rom_size_to_number_of_banks = {
    {0, 2},{1, 4},{2, 8},{3, 16},{4, 32},{5, 64},{6, 128},{7, 256},{8, 512},{0x52, 72},{0x53, 80},{0x54, 96}
};
std::unordered_map<MBC_type, void (Memory::*)(MBC_action, u16, u8, MBC_result*)> mbc_handlers = {
    {MBC_type::MBC1, &Memory::MBC1_handler},
    {MBC_type::MBC2, &Memory::MBC2_handler},
    {MBC_type::MBC3, &Memory::MBC3_handler},
    {MBC_type::MBC5, &Memory::MBC5_handler},
};
std::unordered_set<u8> cart_with_battery = {0x03, 0x06, 0x09, 0x0D, 0x0F, 0x10, 0x12, 0x13, 0x1B, 0x1E, 0x22, 0xFC, 0xFD, 0xFF};
std::unordered_map<u8,MBC_type> cart_type_to_mbc = {
    {0x00, MBC_type::NONE},
    {0x01, MBC_type::MBC1},
    {0x02, MBC_type::MBC1},
    {0x03, MBC_type::MBC1},
    {0x05, MBC_type::MBC2},
    {0x06, MBC_type::MBC2},
    {0x08, MBC_type::NONE},
    {0x09, MBC_type::NONE},
    {0x0B, MBC_type::MMM01},
    {0x0C, MBC_type::MMM01},
    {0x0D, MBC_type::MMM01},
    {0x0F, MBC_type::MBC3},
    {0x10, MBC_type::MBC3},
    {0x11, MBC_type::MBC3},
    {0x12, MBC_type::MBC3},
    {0x13, MBC_type::MBC3},
    {0x19, MBC_type::MBC5},
    {0x1A, MBC_type::MBC5},
    {0x1B, MBC_type::MBC5},
    {0x1C, MBC_type::MBC5},
    {0x1D, MBC_type::MBC5},
    {0x1E, MBC_type::MBC5},
    {0x20, MBC_type::MBC6},
    {0x22, MBC_type::MBC7},
    {0xFC, MBC_type::CAMERA},
    {0xFD, MBC_type::TAMA5},
    {0xFE, MBC_type::HuC3},
    {0xFF, MBC_type::HuC1}
};
MBC_type get_MBC(u8 cart_type){
    if(cart_type_to_mbc.find(cart_type) != cart_type_to_mbc.end()){
        return cart_type_to_mbc[cart_type];
    }
    return MBC_type::NONE;
}
bool Memory::load_rom(const char* filename) { //Loads the rom from the file. Also sets up the RAM (with save if applicable) and the header
    _rom_filename = strdup(filename);
    std::scoped_lock<std::mutex> lock(this->mem_mutex);
    FILE* file = fopen(filename, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek(file, 0, SEEK_SET);
        if (_rom) {
            free(_rom);
            _rom = nullptr;
            _rom_size = 0;
        }
        if(_ram){
            free(_ram);
            _ram = nullptr;
            _ram_size = 0;
        }
        _rom = (u8*)malloc(size);
        _rom_size = static_cast<size_t>(size);
        fread(_rom, 1, size, file);
        fclose(file);

        this->rom_header = (Cart_header*)(_rom + 0x100);
        this->rom_header->title[15] = 0;
        memcpy(_mem, _rom, 0x8000);
        this->mbc_type = get_MBC(this->rom_header->cart_type);
        if(ram_size_to_bytes[this->rom_header->ram_size] > 0)
            _ram = (u8*)malloc(ram_size_to_bytes[this->rom_header->ram_size]);
        _ram_size = ram_size_to_bytes[this->rom_header->ram_size];
        this->cart_features.has_battery = cart_with_battery.find(this->rom_header->cart_type) != cart_with_battery.end();

        printf("MBC type: %s\n", mbc_names[this->mbc_type].c_str());
        if(this->mbc_type != MBC_type::NONE)
            (this->*mbc_handlers[this->mbc_type])(MBC_action::INIT, 0, 0, nullptr);

        if(this->cart_features.has_battery){
            this->load_save();
        }
        return true;
    }
    return false;

}
void Memory::load_save(){
    std::string save_filename = _rom_filename.substr(0, _rom_filename.find_last_of('.')) + ".sav";
    FILE* file = fopen(save_filename.c_str(), "rb");
    if (file) {
        printf("Save file found, loading...\n");
        fread(_ram, 1, _ram_size, file);
        memcpy(_mem + 0xA000, _ram, std::min(_ram_size, static_cast<size_t>(0x2000)));
        fclose(file);
    }
}
void Memory::save_ram(){
    if(!this->cart_features.has_battery || !_ram) return;
    memcpy(_ram + (current_ram_bank * _ram_bank_size), _mem + 0xA000, _ram_bank_size);
    std::string save_filename = _rom_filename.substr(0, _rom_filename.find_last_of('.')) + ".sav";
    FILE* file = fopen(save_filename.c_str(), "wb");
    if (file) {
        fwrite(_ram, 1, _ram_size, file);
        fclose(file);
    }
}
Cart_header Memory::get_cart_header(){
    std::scoped_lock<std::mutex> lock(this->mem_mutex);
    return this->rom_header ? *this->rom_header : Cart_header{};
}

std::string Memory::get_sha256() {
    std::scoped_lock<std::mutex> lock(this->mem_mutex);
    if (_rom == nullptr || _rom_size == 0) {
        return "";
    }
    char hex_str[SHA256_HEX_SIZE];
    sha256_hex(_rom, _rom_size, hex_str);
    return std::string(hex_str);
}

MBC_result Memory::process_MBC_write(u16 address, u8 data){
    MBC_result result = {MBC_ret_type::DISCARD, 0};
    if(this->mbc_type == MBC_type::NONE) return result;
    (this->*mbc_handlers[this->mbc_type])(MBC_action::WRITE, address, data, &result);
    return result;
}

MBC_result Memory::process_MBC_read(u16 address){
    MBC_result result = {MBC_ret_type::DISCARD, 0};
    if(this->mbc_type == MBC_type::NONE) return result;
    (this->*mbc_handlers[this->mbc_type])(MBC_action::READ, address, 0, &result);
    return result;
}
void Memory::change_banks(size_t new_rom0_bank, size_t new_rom1_bank, size_t new_ram_bank){
    if(_rom){
        if(this->current_rom0_bank != new_rom0_bank){
            memcpy(_mem, _rom + (new_rom0_bank * 0x4000), 0x4000);
            this->current_rom0_bank = new_rom0_bank;
        }
        if(this->current_rom1_bank != new_rom1_bank){
            memcpy(_mem + 0x4000, _rom + (new_rom1_bank * 0x4000), 0x4000);
            this->current_rom1_bank = new_rom1_bank;
        }
    }
    if(_ram && current_ram_bank != new_ram_bank){
        memcpy(_ram + (current_ram_bank * _ram_bank_size), _mem + 0xA000, _ram_bank_size);
        memcpy(_mem + 0xA000, _ram + (new_ram_bank * _ram_bank_size), _ram_bank_size);
        this->current_ram_bank = new_ram_bank;
    }
}
void Memory::MBC1_handler(MBC_action action, u16 address, u8 data, MBC_result* result){
    switch(action){
        case MBC_action::INIT:{
            this->mbc_state = new MBC1_state();
            auto* mbc1 = static_cast<MBC1_state*>(this->mbc_state);
            mbc1->ext_ram_enabled = false;
            mbc1->advanced_banking_mode = false;
            mbc1->rom_banks = rom_size_to_number_of_banks[this->rom_header->rom_size];
            mbc1->ram_banks = ram_size_to_bytes[this->rom_header->ram_size] / 0x2000;
            mbc1->reg_2000_3FFF = 1;
            mbc1->reg_2000_3FFF_mask = static_cast<u8>((1 << static_cast<u8>(std::ceil(std::log2(mbc1->rom_banks)))) - 1);
            if (mbc1->reg_2000_3FFF_mask > 0b00011111) mbc1->reg_2000_3FFF_mask = 0b00011111;
            mbc1->reg_4000_5FFF = 0;
            
            break;
        }
        case MBC_action::READ:{
            auto* mbc1 = static_cast<MBC1_state*>(this->mbc_state);
            if(!mbc1->ext_ram_enabled && BETWEEN(address, 0xA000,0xBFFF)){
                result->type = MBC_ret_type::RETURN;
                result->data = 0xFF;
            }
            break;
        }
        case MBC_action::WRITE:{
            auto* mbc1 = static_cast<MBC1_state*>(this->mbc_state);
            if(BETWEEN(address, 0x0000, 0x1FFF)){
                mbc1->ext_ram_enabled = (data & 0xF) == 0xA;
            }else if (BETWEEN(address, 0x2000, 0x3FFF)){
                data&= 0b00011111;
                if(data == 0) data = 1;
                data &= mbc1->reg_2000_3FFF_mask;
                mbc1->reg_2000_3FFF = data;
                MBC1_change_banks();
            }else if (BETWEEN(address, 0x4000, 0x5FFF)){
                mbc1->reg_4000_5FFF = data & 0b11;
                MBC1_change_banks();
            }else if (BETWEEN(address, 0x6000, 0x7FFF)){
                mbc1->advanced_banking_mode = data & 1;
                MBC1_change_banks();
            }else if(!mbc1->ext_ram_enabled && BETWEEN(address, 0xA000,0xBFFF)){
                result->type = MBC_ret_type::RETURN;
                result->data = 0xFF;
            }
            break;
        }
        default:{
            break;
        }
    }
}
void Memory::MBC1_change_banks(){
    auto* mbc1 = static_cast<MBC1_state*>(this->mbc_state);
    size_t new_rom0_bank, new_rom1_bank, new_ram_bank;
    if(mbc1->advanced_banking_mode){
        new_rom0_bank= (mbc1->reg_4000_5FFF << 5) % mbc1->rom_banks;
        new_rom1_bank = ((mbc1->reg_4000_5FFF << 5) | mbc1->reg_2000_3FFF) % mbc1->rom_banks;
        new_ram_bank = mbc1->reg_4000_5FFF;
    }else{
        new_rom0_bank = 0;
        new_rom1_bank = ((mbc1->reg_4000_5FFF << 5) | mbc1->reg_2000_3FFF) % mbc1->rom_banks;
        new_ram_bank = 0;
    }
    change_banks(new_rom0_bank, new_rom1_bank, new_ram_bank);
}
void Memory::MBC2_handler(MBC_action action, u16 address, u8 data, MBC_result* result){
}
void Memory::MBC3_handler(MBC_action action, u16 address, u8 data, MBC_result* result){
    //TODO: Implement MBC3 RTC
    switch(action){
        case MBC_action::INIT:{
            this->mbc_state = new MBC3_state();
            auto* mbc3 = static_cast<MBC3_state*>(this->mbc_state);
            mbc3->ext_ram_enabled = false;
            mbc3->rom_banks = rom_size_to_number_of_banks[this->rom_header->rom_size];
            mbc3->ram_banks = ram_size_to_bytes[this->rom_header->ram_size] / 0x2000;
            break;
        }
        case MBC_action::READ:{
            auto* mbc3 = static_cast<MBC3_state*>(this->mbc_state);
            if(!mbc3->ext_ram_enabled && BETWEEN(address, 0xA000,0xBFFF)){
                result->type = MBC_ret_type::RETURN;
                result->data = 0xFF;
            }
            break;
        }
        case MBC_action::WRITE:{
            auto* mbc3 = static_cast<MBC3_state*>(this->mbc_state);
            if(BETWEEN(address, 0x0000, 0x1FFF)){
                mbc3->ext_ram_enabled = (data & 0xF) == 0xA;
            }else if(BETWEEN(address, 0x2000, 0x3FFF)){
                u8 new_rom1_bank = data & 0b01111111;
                if(new_rom1_bank == 0) new_rom1_bank = 1;
                change_banks(0, new_rom1_bank % mbc3->rom_banks, current_ram_bank);
            }else if(BETWEEN(address, 0x4000, 0x5FFF)){
                if(data<8)
                    change_banks(0, current_rom1_bank, mbc3->ram_banks == 0? 0: data % mbc3->ram_banks);
                //TODO: RTC registers
            }else if(!mbc3->ext_ram_enabled && BETWEEN(address, 0xA000,0xBFFF)){
                result->type = MBC_ret_type::RETURN;
                result->data = data;
            }

        }
    }
}
void Memory::MBC5_handler(MBC_action action, u16 address, u8 data, MBC_result* result){
    switch(action){
        case MBC_action::INIT:{
            this->mbc_state = new MBC5_state();
            auto* mbc5 = static_cast<MBC5_state*>(this->mbc_state);
            mbc5->ext_ram_enabled = false;
            mbc5->rom_banks = rom_size_to_number_of_banks[this->rom_header->rom_size];
            mbc5->ram_banks = ram_size_to_bytes[this->rom_header->ram_size] / 0x2000;
            mbc5->reg_2000_2FFF = 0;
            mbc5->reg_3000_3FFF = 0;
            mbc5->reg_4000_5FFF = 0;
            printf("MBC5 ROM banks: %zu\n", mbc5->rom_banks);
            printf("MBC5 RAM banks: %zu\n", mbc5->ram_banks);
            break;
        }
        case MBC_action::READ:{
            auto* mbc5 = static_cast<MBC5_state*>(this->mbc_state);
            if(!mbc5->ext_ram_enabled && BETWEEN(address, 0xA000,0xBFFF)){
                result->type = MBC_ret_type::RETURN;
                result->data = 0xFF;
            }
            break;
        }
        case MBC_action::WRITE:{
            auto* mbc5 = static_cast<MBC5_state*>(this->mbc_state);
            if(BETWEEN(address, 0x0000, 0x1FFF)){
                // if((data & 0xF) == 0xA){
                //     mbc5->ext_ram_enabled = true;
                // }else if(data == 0){
                //     mbc5->ext_ram_enabled = false;
                // }
                mbc5->ext_ram_enabled = (data & 0xF) == 0xA;
            }else if(BETWEEN(address, 0x2000, 0x2FFF)){
                mbc5->reg_2000_2FFF = data;
                MBC5_change_banks();
            }else if(BETWEEN(address, 0x3000, 0x3FFF)){
                mbc5->reg_3000_3FFF = data & 0b1;
                MBC5_change_banks();
            }else if(BETWEEN(address, 0x4000, 0x5FFF)){
                mbc5->reg_4000_5FFF = data & 0b1111;
                MBC5_change_banks();
                //TODO: rumble
                
            }else if(!mbc5->ext_ram_enabled && BETWEEN(address, 0xA000,0xBFFF)){
                result->type = MBC_ret_type::RETURN;
                result->data = data;
            }
            break;
        }
        
    }
}
void Memory::MBC5_change_banks(){
    auto* mbc5 = static_cast<MBC5_state*>(this->mbc_state);
    size_t new_rom0_bank = 0;
    size_t new_rom1_bank = ((mbc5->reg_3000_3FFF << 8) | mbc5->reg_2000_2FFF) % mbc5->rom_banks;
    size_t new_ram_bank = mbc5->ram_banks == 0 ? 0 : mbc5->reg_4000_5FFF % mbc5->ram_banks;
    change_banks(new_rom0_bank, new_rom1_bank, new_ram_bank);
}