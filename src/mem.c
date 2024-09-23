#include <mem.h>
#include <utils.h>
void mem_write(Memory *mem, u16 addr, u8 data, bool fromCPU){
    mem->mem[addr] = data;
}
void mem_write(Memory *mem, u16 addr, u8 data){
    mem_write(mem, addr, data, true);
}
void mem_write16(Memory *mem, u16 addr, u16 data, bool fromCPU){
    mem_write(mem, addr, data & 0xFF, fromCPU);
    mem_write(mem, addr + 1, data >> 8, fromCPU);
}
void mem_write16(Memory *mem, u16 addr, u16 data){
    mem_write16(mem, addr, data, true);
}

u8 mem_read(Memory *mem, u16 addr, bool fromCPU){
    return mem->mem[addr];
}
u8 mem_read(Memory *mem, u16 addr){
    return mem_read(mem, addr, true);
}
u16 mem_read16(Memory *mem, u16 addr, bool fromCPU){
    return mem_read(mem, addr, fromCPU) | (mem_read(mem, addr + 1, fromCPU) << 8);
}
u16 mem_read16(Memory *mem, u16 addr){
    return mem_read16(mem, addr, true);
}