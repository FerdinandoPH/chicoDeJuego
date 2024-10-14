#pragma once

#include <utils.h>
#include <instr.h>
#include <mem.h>
#include <emu.h>

void cpuInit();
bool cpuStep();

typedef struct{
    u8 a, f, b, c, d, e, h, l;
    u16 sp, pc;
} Registers;

typedef struct{
    Registers regs;
    u8 opcode;
    u16 opSrc;
    bool toMem;
    u16 destAddr;
    Instr* instr;
    bool IME;
    bool IME_scheduled;
} CpuData;

typedef void (*InstrFunc)(CpuData*);

#define FLAG_Z GET_BIT(cpuData.regs.f, 7)
#define FLAG_N GET_BIT(cpuData.regs.f, 6)
#define FLAG_H GET_BIT(cpuData.regs.f, 5)
#define FLAG_C GET_BIT(cpuData.regs.f, 4)
typedef enum{
    NONE,
    A,
    B,
    C,
    D,
    E,
    H,
    L,
    AF,
    BC,
    DE,
    HL,
    SP,
    PC
} RegType;
u16 getReg16(RegType reg);
void setReg16(RegType reg, u16 value);

char* printState();