#pragma once
#include <utils.h>
#include <cpu.h>
typedef enum{
    //src in right when applicable
    IMPL,
    N8,
    R,
    R_R,
    MEMFROMREG_R,
    R_MEMFROMREG
} AddrMode;



typedef enum{
    EMPTY,
    NOP,
    LD,

} InstrProc;

typedef enum{
    ALWAYS,
    Z,
    C,
    NZ,
    NC
} InstrCond;

typedef struct{
    InstrProc proc;
    AddrMode addrMode;
    RegType regDest;
    RegType regSrc;
    InstrCond cond;
    u8 param;
} Instr;

Instr* opcodeToInstr(u8 opcode);
char* getInstrName(InstrProc t);