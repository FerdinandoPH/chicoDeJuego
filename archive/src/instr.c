#include <instr.h>

Instr instructions [0x100]={
    [0x00] = {NOP, IMPL},


    [0x40] = {LD, R_R, B, B},
    [0x41] = {LD, R_R, B, C},
    [0x42] = {LD, R_R, B, D},
    [0x43] = {LD, R_R, B, E},
    [0x44] = {LD, R_R, B, H},
    [0x45] = {LD, R_R, B, L},
    [0x46] = {LD, R_MEMFROMREG, B, HL},
    [0x47] = {LD, R_R, B, A},
    [0x48] = {LD, R_R, C, B},
    [0x49] = {LD, R_R, C, C},
    [0x4A] = {LD, R_R, C, D},
    [0x4B] = {LD, R_R, C, E},
    [0x4C] = {LD, R_R, C, H},
    [0x4D] = {LD, R_R, C, L},
    [0x4E] = {LD, R_MEMFROMREG, C, HL},
    [0x4F] = {LD, R_R, C, A},

    [0x50] = {LD, R_R, D, B},
    [0x51] = {LD, R_R, D, C},
    [0x52] = {LD, R_R, D, D},
    [0x53] = {LD, R_R, D, E},
    [0x54] = {LD, R_R, D, H},
    [0x55] = {LD, R_R, D, L},
    [0x56] = {LD, R_MEMFROMREG, D, HL},
    [0x57] = {LD, R_R, D, A},
    [0x58] = {LD, R_R, E, B},
    [0x59] = {LD, R_R, E, C},
    [0x5A] = {LD, R_R, E, D},
    [0x5B] = {LD, R_R, E, E},
    [0x5C] = {LD, R_R, E, H},
    [0x5D] = {LD, R_R, E, L},
    [0x5E] = {LD, R_MEMFROMREG, E, HL},
    [0x5F] = {LD, R_R, E, A},

    [0x60] = {LD, R_R, H, B},
    [0x61] = {LD, R_R, H, C},
    [0x62] = {LD, R_R, H, D},
    [0x63] = {LD, R_R, H, E},
    [0x64] = {LD, R_R, H, H},
    [0x65] = {LD, R_R, H, L},
    [0x66] = {LD, R_MEMFROMREG, H, HL},
    [0x67] = {LD, R_R, H, A},
    [0x68] = {LD, R_R, L, B},
    [0x69] = {LD, R_R, L, C},
    [0x6A] = {LD, R_R, L, D},
    [0x6B] = {LD, R_R, L, E},
    [0x6C] = {LD, R_R, L, H},
    [0x6D] = {LD, R_R, L, L},
    [0x6E] = {LD, R_MEMFROMREG, L, HL},
    [0x6F] = {LD, R_R, L, A},

    [0x70] = {LD, MEMFROMREG_R, HL, B},
    [0x71] = {LD, MEMFROMREG_R, HL, C},
    [0x72] = {LD, MEMFROMREG_R, HL, D},
    [0x73] = {LD, MEMFROMREG_R, HL, E},
    [0x74] = {LD, MEMFROMREG_R, HL, H},
    [0x75] = {LD, MEMFROMREG_R, HL, L},
    [0x77] = {LD, MEMFROMREG_R, HL, A},

    [0x78] = {LD, R_R, A, B},
    [0x79] = {LD, R_R, A, C},
    [0x7A] = {LD, R_R, A, D},
    [0x7B] = {LD, R_R, A, E},
    [0x7C] = {LD, R_R, A, H},
    [0x7D] = {LD, R_R, A, L},
    [0x7E] = {LD, R_MEMFROMREG, A, HL},
    [0x7F] = {LD, R_R, A, A}
};

char* instrNames[] = {
    "EMPTY",
    "NOP",
    "LD"
};

char* getInstrName(InstrProc t){
    return instrNames[t];
}

Instr* opcodeToInstr(u8 opcode){
    return &instructions[opcode];
}