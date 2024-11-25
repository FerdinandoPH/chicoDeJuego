#include <memory.h>
#include <utils.h>
#include <cstring>
#include <cstdio>
#include <stdlib.h>
Memory::Memory() {
    memset(_mem, 0, sizeof(_mem));
}
Memory::~Memory() {
}
void Memory::write(uint16_t address, uint8_t data, bool from_cpu) {
    _mem[address] = data;
}

uint8_t Memory::read(uint16_t address, bool from_cpu) {
    return _mem[address];
}

uint8_t Memory::readX(uint16_t address) {
    return this->read(address, false);
}

void Memory::writeX(uint16_t address, uint8_t data) {
    this->write(address, data, false);
}

bool Memory::load_rom(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek(file, 0, SEEK_SET);
        if (_rom) {
            free(_rom);
        }
        _rom = (uint8_t*)malloc(size);
        fread(_rom, 1, size, file);
        fclose(file);
        this->rom_header = (Cart_header*)(_rom + 0x100);
        this->rom_header->title[15] = 0;
        memcpy(_mem, _rom, 0x8000);
        return true;
    }
    return false;
}

void Memory::dump() {
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