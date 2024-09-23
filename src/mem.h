#pragma once
#include <utils.h>
typedef struct{
    u8 mem[0x10000];
}Memory;

void mem_write(Memory *mem, u16 addr, u8 data, bool fromCPU);
void mem_write(Memory *mem, u16 addr, u8 data);
void mem_write16(Memory *mem, u16 addr, u16 data, bool fromCPU);
void mem_write16(Memory *mem, u16 addr, u16 data);

u8 mem_read(Memory *mem, u16 addr, bool fromCPU);
u8 mem_read(Memory *mem, u16 addr);
u16 mem_read16(Memory *mem, u16 addr, bool fromCPU);
u16 mem_read16(Memory *mem, u16 addr);