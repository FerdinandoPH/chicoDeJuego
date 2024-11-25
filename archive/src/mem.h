#pragma once
#include <utils.h>

typedef struct{
    u8 mem[0x10000];
}Memory;

void memWrite(Memory *mem, u16 addr, u8 data, bool fromCPU);
void memWriteU(Memory *mem, u16 addr, u8 data);
void memWrite16(Memory *mem, u16 addr, u16 data, bool fromCPU);
void memWrite16U(Memory *mem, u16 addr, u16 data);

u8 memRead(Memory *mem, u16 addr, bool fromCPU);
u8 memReadU(Memory *mem, u16 addr);
u16 memRead16(Memory *mem, u16 addr, bool fromCPU);
u16 memRead16U(Memory *mem, u16 addr);

void memDump(Memory *mem);