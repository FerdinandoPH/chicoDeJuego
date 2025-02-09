#include <memory.h>
#include <utils.h>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <stdlib.h>
Memory::Memory() : mutex() {
    memset(_mem, 0, sizeof(_mem));
    this->reset();
}
Memory::~Memory() {
    if (_rom) {
        free(_rom);
    }
}
/*When reading/writing to memory from CPU, sometimes some side effects will occur
 *readX and writeX will ignore these side effects*/
void Memory::write(u16 address, u8 data, bool from_cpu) {
    this->mutex.lock();
    from_cpu = from_cpu && this->is_protected;
    if (from_cpu){
        u16 write_zero [] = {0xFF04}; //DIV
        if (std::find(std::begin(write_zero), std::end(write_zero), address) != std::end(write_zero)){
            data = 0;
        }
    }
    _mem[address] = data;
    this->mutex.unlock();
}

u8 Memory::read(u16 address, bool from_cpu) {
    this->mutex.lock();
    from_cpu = from_cpu && this->is_protected;
    u8 data = _mem[address];
    this->mutex.unlock();
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


bool Memory::load_rom(const char* filename) { //Loads the rom to the file (deletes previous rom)
    this->mutex.lock();
    FILE* file = fopen(filename, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek(file, 0, SEEK_SET);
        if (_rom) {
            free(_rom);
        }
        _rom = (u8*)malloc(size);
        fread(_rom, 1, size, file);
        fclose(file);
        this->rom_header = (Cart_header*)(_rom + 0x100);
        this->rom_header->title[15] = 0;
        memcpy(_mem, _rom, 0x8000);
        this->mutex.unlock();
        return true;
    }
    this->mutex.unlock();
    return false;

}

void Memory::dump() { //Writes the current state of memory into a file and opens it with a HEX editor
    this->mutex.lock();
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
    this->mutex.unlock();
}

void Memory::reset(){
    this->mutex.lock();
    memset(_mem, 0, sizeof(_mem));
    if (_rom != NULL)
        memcpy(_mem, _rom, 0x8000);
    this->mutex.unlock();
}

void Memory::load_initial_values(){
    _mem[0xFF41] = 0x85; //LCD STAT
}