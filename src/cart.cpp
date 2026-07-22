#include "memory.h"
#include <stdlib.h>
#include <cmath>
#include "sha256.h"
const std::unordered_map<u8,size_t> cart_ram_size_to_bytes = {
    {0, 0},{1, 0},{2, 8*1024},{3, 32*1024},{4, 128*1024},{5, 64*1024}
};
const std::unordered_map<u8,size_t> rom_size_to_number_of_banks = {
    {0, 2},{1, 4},{2, 8},{3, 16},{4, 32},{5, 64},{6, 128},{7, 256},{8, 512},{0x52, 72},{0x53, 80},{0x54, 96}
};
const std::unordered_map<MBC_type, void (Memory::*)(MBC_action, u16, u8, MBC_result*)> mbc_handlers = {
    {MBC_type::MBC1, &Memory::MBC1_handler},
    {MBC_type::MBC2, &Memory::MBC2_handler},
    {MBC_type::MBC3, &Memory::MBC3_handler},
    {MBC_type::MBC5, &Memory::MBC5_handler},
};
const std::unordered_set<u8> cart_with_battery = {0x03, 0x06, 0x09, 0x0D, 0x0F, 0x10, 0x12, 0x13, 0x1B, 0x1E, 0x22, 0xFC, 0xFD, 0xFF};
const std::unordered_map<u8,MBC_type> cart_type_to_mbc = {
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
        return cart_type_to_mbc.at(cart_type);
    }
    return MBC_type::NONE;
}


void rtc_update(Rtc_state* rtc){
    auto now = std::chrono::system_clock::now();
    if(rtc->regs.is_stopped){
        //While halted no time is accumulated, so the fraction is dropped on purpose
        rtc->last_write_time = now;
        return;
    }
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - rtc->last_write_time).count();
    if(elapsed_seconds <= 0) return; //Keep the sub-second remainder for the next call
    rtc->last_write_time += std::chrono::seconds(elapsed_seconds);

    if(rtc->regs.seconds > 59){
        if(elapsed_seconds < 64 - rtc->regs.seconds){
            rtc->regs.seconds += elapsed_seconds;
            return;
        }
        elapsed_seconds -= (64 - rtc->regs.seconds);
        rtc->regs.seconds = 0;
    }
    auto elapsed_minutes = elapsed_seconds / 60;
    if (rtc->regs.seconds + (elapsed_seconds % 60) > 59) {
        elapsed_minutes += 1;
    }
    rtc->regs.seconds = (rtc->regs.seconds + (elapsed_seconds % 60)) % 60;


    if(rtc->regs.minutes > 59){
        if(elapsed_minutes < 64 - rtc->regs.minutes){
            rtc->regs.minutes += elapsed_minutes;
            return;
        }
        elapsed_minutes -= (64 - rtc->regs.minutes);
        rtc->regs.minutes = 0;
    }
    auto elapsed_hours = elapsed_minutes / 60;
    if(rtc->regs.minutes + (elapsed_minutes % 60) > 59) {
        elapsed_hours += 1;
    }
    rtc->regs.minutes = (rtc->regs.minutes + (elapsed_minutes % 60)) % 60;


    if(rtc->regs.hours > 23){
        if(elapsed_hours < 32 - rtc->regs.hours){
            rtc->regs.hours += elapsed_hours;
            return;
        }
        elapsed_hours -= (32 - rtc->regs.hours);
        rtc->regs.hours = 0;
    }
    auto elapsed_days = elapsed_hours / 24;
    if(rtc->regs.hours + (elapsed_hours % 24) > 23) {
        elapsed_days += 1;
    }
    rtc->regs.hours = (rtc->regs.hours + (elapsed_hours % 24)) % 24;

    long long rtc_days = (rtc->regs.days_hi & 0x01) * 256 + rtc->regs.days_lo;
    if (rtc_days + elapsed_days > 511){
        rtc->regs.day_carry = true;
        rtc->regs.days_hi |= 0x80; // Set bit 7 to indicate day carry
    }
    rtc_days = (rtc_days + elapsed_days) % 512;
    rtc->regs.days_lo = rtc_days & 0xFF;
    rtc->regs.days_hi = (rtc->regs.days_hi & 0xFE) | ((rtc_days >> 8) & 0x01);


}
bool Memory::load_rom(const char* filename) { //Loads the rom from the file. Also sets up the RAM (with save if applicable) and the header
    _rom_filename = strdup(filename);
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
        if(_cart_ram){
            free(_cart_ram);
            _cart_ram = nullptr;
            _cart_ram_size = 0;
        }
        _rom = (u8*)malloc(size);
        _rom_size = static_cast<size_t>(size);
        fread(_rom, 1, size, file);
        fclose(file);

        this->rom_header = (Cart_header*)(_rom + 0x100);
        //Byte 15 is actually the GBC flag too
        if((this->rom_header->title[15] == 0x80 || this->rom_header->title[15] == 0xC0) && !prefs->force_dmg)
            this->gb_model = GB_model::CGB;
        this->rom_header->title[15] = 0;
        memcpy(_mem, _rom, 0x8000);
        
        this->mbc_type = get_MBC(this->rom_header->cart_type);
        if(cart_ram_size_to_bytes.at(this->rom_header->cart_ram_size) > 0)
            _cart_ram = (u8*)malloc(cart_ram_size_to_bytes.at(this->rom_header->cart_ram_size));
        _cart_ram_size = cart_ram_size_to_bytes.at(this->rom_header->cart_ram_size);
        this->cart_features.has_battery = cart_with_battery.find(this->rom_header->cart_type) != cart_with_battery.end();

        printf("MBC type: %s\n", mbc_names.at(this->mbc_type).c_str());
        if(this->mbc_type != MBC_type::NONE)
            (this->*mbc_handlers.at(this->mbc_type))(MBC_action::INIT, 0, 0, nullptr);

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
        if (this->mbc_type == MBC_type::MBC2){
            fread(std::get_if<MBC2_state>(&this->mbc_state)->mbc2_ram, 1, 512, file);
        }else{
            fread(_cart_ram, 1, _cart_ram_size, file);
            memcpy(_mem + 0xA000, _cart_ram, std::min(_cart_ram_size, static_cast<size_t>(0x2000)));
        }
        fclose(file);
    }
}
void Memory::save_cart_ram(){
    if(!this->cart_features.has_battery || !_cart_ram) return;
    memcpy(_cart_ram + (current_cart_ram_bank * _cart_ram_bank_size), _mem + 0xA000, _cart_ram_bank_size);
    std::string save_filename = _rom_filename.substr(0, _rom_filename.find_last_of('.')) + ".sav";
    FILE* file = fopen(save_filename.c_str(), "wb");
    if (file) {
        fwrite(_cart_ram, 1, _cart_ram_size, file);
        fclose(file);
    }
}
Cart_header Memory::get_cart_header(){
    return this->rom_header ? *this->rom_header : Cart_header{};
}

std::string Memory::get_sha256() {
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
    (this->*mbc_handlers.at(this->mbc_type))(MBC_action::WRITE, address, data, &result);
    return result;
}

MBC_result Memory::process_MBC_read(u16 address){
    MBC_result result = {MBC_ret_type::DISCARD, 0};
    if(this->mbc_type == MBC_type::NONE) return result;
    (this->*mbc_handlers.at(this->mbc_type))(MBC_action::READ, address, 0, &result);
    return result;
}
void Memory::change_banks(size_t new_rom0_bank, size_t new_rom1_bank, size_t new_cart_ram_bank, bool dump_cart_ram){
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
    if(_cart_ram && current_cart_ram_bank != new_cart_ram_bank){
        if(dump_cart_ram)
            memcpy(_cart_ram + (current_cart_ram_bank * _cart_ram_bank_size), _mem + 0xA000, _cart_ram_bank_size);
        memcpy(_mem + 0xA000, _cart_ram + (new_cart_ram_bank * _cart_ram_bank_size), _cart_ram_bank_size);
        this->current_cart_ram_bank = new_cart_ram_bank;
    }
}
void Memory::MBC1_handler(MBC_action action, u16 address, u8 data, MBC_result* result){
    switch(action){
        case MBC_action::INIT:{
            this->mbc_state.emplace<MBC1_state>();
            auto* mbc1 = std::get_if<MBC1_state>(&this->mbc_state);
            mbc1->ext_ram_enabled = false;
            mbc1->advanced_banking_mode = false;
            mbc1->rom_banks = rom_size_to_number_of_banks.at(this->rom_header->rom_size);
            mbc1->cart_ram_banks = cart_ram_size_to_bytes.at(this->rom_header->cart_ram_size) / 0x2000;
            mbc1->reg_2000_3FFF = 1;
            mbc1->reg_2000_3FFF_mask = static_cast<u8>((1 << static_cast<u8>(std::ceil(std::log2(mbc1->rom_banks)))) - 1);
            if (mbc1->reg_2000_3FFF_mask > 0b00011111) mbc1->reg_2000_3FFF_mask = 0b00011111;
            mbc1->reg_4000_5FFF = 0;

            break;
        }
        case MBC_action::READ:{
            auto* mbc1 = std::get_if<MBC1_state>(&this->mbc_state);
            if(!mbc1->ext_ram_enabled && BETWEEN(address, 0xA000,0xBFFF)){
                result->type = MBC_ret_type::RETURN;
                result->data = 0xFF;
            }
            break;
        }
        case MBC_action::WRITE:{
            auto* mbc1 = std::get_if<MBC1_state>(&this->mbc_state);
            if(BETWEEN(address, 0x0000, 0x1FFF)){
                mbc1->ext_ram_enabled = (data & 0xF) == 0xA;
            }else if (BETWEEN(address, 0x2000, 0x3FFF)){
                data&= 0b00011111;
                data &= mbc1->reg_2000_3FFF_mask;
                if(data == 0) data = 1;
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
    auto* mbc1 = std::get_if<MBC1_state>(&this->mbc_state);
    size_t new_rom0_bank, new_rom1_bank, new_ram_bank;
    if(mbc1->advanced_banking_mode){
        new_rom0_bank= (mbc1->reg_4000_5FFF << 5) % mbc1->rom_banks;
        new_rom1_bank = ((mbc1->reg_4000_5FFF << 5) | mbc1->reg_2000_3FFF) % mbc1->rom_banks;
        new_ram_bank = mbc1->reg_4000_5FFF % mbc1->cart_ram_banks;
    }else{
        new_rom0_bank = 0;
        new_rom1_bank = ((mbc1->reg_4000_5FFF << 5) | mbc1->reg_2000_3FFF) % mbc1->rom_banks;
        new_ram_bank = 0;
    }
    change_banks(new_rom0_bank, new_rom1_bank, new_ram_bank);
}
void Memory::MBC2_handler(MBC_action action, u16 address, u8 data, MBC_result* result){
    switch(action){
        case MBC_action::INIT:{
            this->mbc_state.emplace<MBC2_state>();
            auto* mbc2 = std::get_if<MBC2_state>(&this->mbc_state);
            mbc2->ext_ram_enabled = false;
            mbc2->rom_banks = rom_size_to_number_of_banks.at(this->rom_header->rom_size);
            mbc2->cart_ram_banks = 1;
            memset(mbc2->mbc2_ram, 0xFF, 512);
            break;
        }
        case MBC_action::READ:{
            auto* mbc2 = std::get_if<MBC2_state>(&this->mbc_state);
            if(BETWEEN(address, 0xA000,0xBFFF)){
                if(!mbc2->ext_ram_enabled){
                    result->type = MBC_ret_type::RETURN;
                    result->data = 0xFF;
                }else{
                    result->type = MBC_ret_type::KEEP;
                    result->data = mbc2->mbc2_ram[address & 0x1FF];
                }
            }
            break;
        }
        case MBC_action::WRITE:{
            if(BETWEEN(address, 0x0000, 0x3FFF)){
                auto* mbc2 = std::get_if<MBC2_state>(&this->mbc_state);
                if (address & 0x0100){ //ROM
                    u8 new_rom1_bank = data & 0xF;
                    if(new_rom1_bank == 0) new_rom1_bank = 1;
                    change_banks(0, new_rom1_bank, 0);
                }else{ //RAM
                    mbc2->ext_ram_enabled = (data & 0xF) == 0xA;
                }
            }else if(BETWEEN(address, 0xA000,0xBFFF)){
                auto* mbc2 = std::get_if<MBC2_state>(&this->mbc_state);
                if(mbc2->ext_ram_enabled){
                    //TODO: can be written during DMA (shouldn't)
                    mbc2->mbc2_ram[address & 0x1FF] = data & 0xF;
                    result->type = MBC_ret_type::RETURN;
                }
            }
            break;
        }
        default:{
            break;
        }
    }
}
void Memory::MBC3_handler(MBC_action action, u16 address, u8 data, MBC_result* result){
    //TODO: Implement MBC3 RTC
    switch(action){
        case MBC_action::INIT:{
            this->mbc_state.emplace<MBC3_state>();
            auto* mbc3 = std::get_if<MBC3_state>(&this->mbc_state);
            mbc3->ext_ram_enabled = false;
            mbc3->rom_banks = rom_size_to_number_of_banks.at(this->rom_header->rom_size);
            mbc3->cart_ram_banks = cart_ram_size_to_bytes.at(this->rom_header->cart_ram_size) / 0x2000;
            
            //If RTC, initialize
            if (cart_features.has_rtc){
                //try reading <game_name>.rtc file, if not found, initialize to 0
                std::string rtc_filename = _rom_filename.substr(0, _rom_filename.find_last_of('.')) + ".rtc";
                FILE* file = fopen(rtc_filename.c_str(), "rb");
                if (file) {
                    printf("RTC file found, loading...\n");
                    fread(&mbc3->rtc_state, 1, sizeof(Rtc_state), file);
                    fclose(file);
                }else{
                    mbc3->rtc_state.regs.seconds = 0;
                    mbc3->rtc_state.regs.minutes = 0;
                    mbc3->rtc_state.regs.hours = 0;
                    mbc3->rtc_state.regs.days_lo = 0;
                    mbc3->rtc_state.regs.days_hi = 0;
                    mbc3->rtc_state.regs.is_stopped = false;
                    mbc3->rtc_state.regs.day_carry = false;
                    mbc3->rtc_state.last_write_time = std::chrono::system_clock::now();
                }
                mbc3->latched_regs = mbc3->rtc_state.regs;
            }
            break;
        }
        case MBC_action::READ:{
            auto* mbc3 = std::get_if<MBC3_state>(&this->mbc_state);
            if(BETWEEN(address, 0xA000,0xBFFF)){
                if(!mbc3->ext_ram_enabled){
                    result->type = MBC_ret_type::RETURN;
                    result->data = 0xFF;
                }
                else if(mbc3->ram_mode == MBC3_ram_mode::RTC){
                    result->type = MBC_ret_type::RETURN;
                    switch(mbc3->reg_4000_5FFF){
                        case 0x08:
                            result->data = mbc3->latched_regs.seconds;
                            break;
                        case 0x09:
                            result->data = mbc3->latched_regs.minutes;
                            break;
                        case 0x0A:
                            result->data = mbc3->latched_regs.hours;
                            break;
                        case 0x0B:
                            result->data = mbc3->latched_regs.days_lo;
                            break;
                        case 0x0C:
                            result->data = mbc3->latched_regs.days_hi;
                            break;
                        default:
                            result->data = 0xFF;
                            break;
                    }
                }
            }
            break;
        }
        case MBC_action::WRITE:{
            auto* mbc3 = std::get_if<MBC3_state>(&this->mbc_state);
            if(BETWEEN(address, 0x0000, 0x1FFF)){
                mbc3->ext_ram_enabled = (data & 0xF) == 0xA;
            }else if(BETWEEN(address, 0x2000, 0x3FFF)){
                u8 new_rom1_bank = data & 0b01111111;
                if(new_rom1_bank == 0) new_rom1_bank = 1;
                change_banks(0, new_rom1_bank % mbc3->rom_banks, current_cart_ram_bank);
            }else if(BETWEEN(address, 0x4000, 0x5FFF)){
                mbc3->reg_4000_5FFF = data;
                if(data<8){
                    mbc3->ram_mode = MBC3_ram_mode::RAM;
                    change_banks(0, current_rom1_bank, mbc3->cart_ram_banks == 0? 0: data % mbc3->cart_ram_banks);
                }
                else if (cart_features.has_rtc){
                    mbc3->ram_mode = MBC3_ram_mode::RTC;
                }
            }else if(BETWEEN(address, 0x6000, 0x7FFF) && cart_features.has_rtc){
                if(data == 1 && mbc3->reg_6000_7FFF == 0){
                    rtc_update(&mbc3->rtc_state);
                    mbc3->latched_regs = mbc3->rtc_state.regs;
                }
                mbc3->reg_6000_7FFF = data;
            }else if(BETWEEN(address, 0xA000,0xBFFF)){
                if (!mbc3->ext_ram_enabled){
                    result->type = MBC_ret_type::RETURN;
                    result->data = data;
                }else if(mbc3->ram_mode == MBC3_ram_mode::RTC){
                    result->type = MBC_ret_type::RETURN;
                    result->data = data;
                    rtc_update(&mbc3->rtc_state);
                    switch(mbc3->reg_4000_5FFF){
                        case 0x08:
                            mbc3->rtc_state.regs.seconds = data & 0x3F;
                            break;
                        case 0x09:
                            mbc3->rtc_state.regs.minutes = data & 0x3F;
                            break;
                        case 0x0A:
                            mbc3->rtc_state.regs.hours = data & 0x1F;
                            break;
                        case 0x0B:
                            mbc3->rtc_state.regs.days_lo = data;
                            break;
                        case 0x0C:
                            mbc3->rtc_state.regs.days_hi = data & 0xC1;
                            mbc3->rtc_state.regs.is_stopped = (data & 0x40) != 0;
                            mbc3->rtc_state.regs.day_carry = (data & 0x80) != 0;
                            break;
                        default:
                            break;
                    }
                }
            }
            break;
        }
        case MBC_action::CLOSE:{
            // Save RTC
            auto* mbc3 = std::get_if<MBC3_state>(&this->mbc_state);
            if (cart_features.has_rtc){
                rtc_update(&mbc3->rtc_state);
                std::string rtc_filename = _rom_filename.substr(0, _rom_filename.find_last_of('.')) + ".rtc";
                FILE* file = fopen(rtc_filename.c_str(), "wb");
                if (file) {
                    fwrite(&mbc3->rtc_state, 1, sizeof(Rtc_state), file);
                    fclose(file);
                }
            }
            break;
        }
        default:{
            break;
        }
    }
}
void Memory::MBC5_handler(MBC_action action, u16 address, u8 data, MBC_result* result){
    switch(action){
        case MBC_action::INIT:{
            this->mbc_state.emplace<MBC5_state>();
            auto* mbc5 = std::get_if<MBC5_state>(&this->mbc_state);
            mbc5->ext_ram_enabled = false;
            mbc5->rom_banks = rom_size_to_number_of_banks.at(this->rom_header->rom_size);
            mbc5->cart_ram_banks = cart_ram_size_to_bytes.at(this->rom_header->cart_ram_size) / 0x2000;
            mbc5->reg_2000_2FFF = 0;
            mbc5->reg_3000_3FFF = 0;
            mbc5->reg_4000_5FFF = 0;
            printf("MBC5 ROM banks: %zu\n", mbc5->rom_banks);
            printf("MBC5 RAM banks: %zu\n", mbc5->cart_ram_banks);
            break;
        }
        case MBC_action::READ:{
            auto* mbc5 = std::get_if<MBC5_state>(&this->mbc_state);
            if(!mbc5->ext_ram_enabled && BETWEEN(address, 0xA000,0xBFFF)){
                result->type = MBC_ret_type::RETURN;
                result->data = 0xFF;
            }
            break;
        }
        case MBC_action::WRITE:{
            auto* mbc5 = std::get_if<MBC5_state>(&this->mbc_state);
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
        default:{
            break;
        }
    }
    
}
void Memory::MBC5_change_banks(){
    auto* mbc5 = std::get_if<MBC5_state>(&this->mbc_state);
    size_t new_rom0_bank = 0;
    size_t new_rom1_bank = ((mbc5->reg_3000_3FFF << 8) | mbc5->reg_2000_2FFF) % mbc5->rom_banks;
    size_t new_cart_ram_bank = mbc5->cart_ram_banks == 0 ? 0 : mbc5->reg_4000_5FFF % mbc5->cart_ram_banks;
    change_banks(new_rom0_bank, new_rom1_bank, new_cart_ram_bank);
}