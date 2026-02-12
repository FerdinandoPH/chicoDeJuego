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
    std::lock_guard<std::mutex> lock(this->mutex);
    bool writable = true;
    from_cpu = from_cpu && this->is_protected;
    if (from_cpu){
        if(address<0x8000){
            std::cout<<"Rom write attempt at address: "<<numToHexString(address, 4)<<" and value: "<<numToHexString(data, 2)<<std::endl;
            //writable = false;
            return;
        }
        if(dma->transferring && !BETWEEN(address, 0xFE00, 0xFE9F)){
            std::cout<<"Writing during DMA transfer at address: "<<numToHexString(address, 4)<<" and value: "<<numToHexString(data, 2)<<std::endl;
            //writable = false;
            return;
        }
        std::unordered_set<u16> write_zero = {DIV};
        if (write_zero.find(address) != write_zero.end()){
            data = 0;
        }
        else if (address == DMA_DIR){
            if (data > 0xDF){
                std::cout<<"Invalid DMA source address: "<<numToHexString(data, 2)<<std::endl;
                data = 0xDF;
            }
            this->dma->start(static_cast<u16>(data)*0x100, 0xFE00, 0xA0);
        }
        
    }
    if(writable)
        _mem[address] = data;
}

u8 Memory::read(u16 address, bool from_cpu) {
    std::lock_guard<std::mutex> lock(this->mutex);
    from_cpu = from_cpu && this->is_protected;
    if(from_cpu){
        if(dma->transferring && !BETWEEN(address, 0xFE00, 0xFE9F)){
            std::cout<<"Reading during DMA transfer at address: "<<numToHexString(address, 4)<<std::endl;
            return 0xFF;
        }
    }
    return _mem[address];
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
    std::lock_guard<std::mutex> lock(this->mutex);
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
        return true;
    }
    return false;

}

void Memory::dump() { //Writes the current state of memory into a file and opens it with a HEX editor
    std::lock_guard<std::mutex> lock(this->mutex);
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
    std::lock_guard<std::mutex> lock(this->mutex);
    memset(_mem, 0, sizeof(_mem));
    if (_rom != NULL)
        memcpy(_mem, _rom, 0x8000);
    this->load_initial_values();
}

void Memory::load_initial_values(){
    _mem[0xFF41] = 0x85; //LCD STAT
}
void Memory::set_dma(Dma* dma){
    this->dma = dma;
}