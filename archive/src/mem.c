#include <mem.h>
#include <utils.h>

void memWrite(Memory *mem, u16 addr, u8 data, bool fromCPU){
    mem->mem[addr] = data;
}
void memWriteU(Memory *mem, u16 addr, u8 data){
    memWrite(mem, addr, data, true);
}
void memWrite16(Memory *mem, u16 addr, u16 data, bool fromCPU){
    memWrite(mem, addr, data & 0xFF, fromCPU);
    memWrite(mem, addr + 1, data >> 8, fromCPU);
}
void memWrite16U(Memory *mem, u16 addr, u16 data){
    memWrite16(mem, addr, data, true);
}

u8 memRead(Memory *mem, u16 addr, bool fromCPU){
    return mem->mem[addr];
}
u8 memReadU(Memory *mem, u16 addr){
    return memRead(mem, addr, true);
}
u16 memRead16(Memory *mem, u16 addr, bool fromCPU){
    return memRead(mem, addr, fromCPU) | (memRead(mem, addr + 1, fromCPU) << 8);
}
u16 memRead16U(Memory *mem, u16 addr){
    return memRead16(mem, addr, true);
}

void memDump(Memory *mem){
        //For debug only
    FILE *file = fopen("mem.hexd", "wb");
    if (file!=NULL){
        size_t writtenData = fwrite(mem->mem, 1, 0x10000, file);
        if (writtenData != 0x10000){
            printf("Error writing mem.hexd\n");
        }
        fclose(file);
        #ifdef _WIN32
            createProcess("mem.hexd");
        #endif
    }
    else{
        printf("Error writing mem.hexd\n");
    }
}