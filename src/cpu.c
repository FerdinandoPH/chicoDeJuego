#include <cpu.h>

CpuData cpuData = {0};
void cpuInit(){
    cpuData.regs.a = 1;
    cpuData.regs.pc = 0x100;
}
static void fetchInstr(){
    //The purpose of forreal is allowing to get the opcode without incrementing the pc
    cpuData.opcode = memReadU(&mem, cpuData.regs.pc);
    cpuData.regs.pc++;
    cpuData.instr = opcodeToInstr(cpuData.opcode);
}
u16 getReg16(RegType reg){
    switch(reg){
        case AF:
            return (cpuData.regs.a << 8) | cpuData.regs.f;
        case BC:
            return (cpuData.regs.b << 8) | cpuData.regs.c;
        case DE:
            return (cpuData.regs.d << 8) | cpuData.regs.e;
        case HL:
            return (cpuData.regs.h << 8) | cpuData.regs.l;
        default:
            printf("Invalid combo register\n");
            return NULL;
    }
}
void setReg16(RegType reg, u16 value){
    switch(reg){
        case AF:
            cpuData.regs.a = value >> 8;
            cpuData.regs.f = value & 0xFF;
            break;
        case BC:
            cpuData.regs.b = value >> 8;
            cpuData.regs.c = value & 0xFF;
            break;
        case DE:
            cpuData.regs.d = value >> 8;
            cpuData.regs.e = value & 0xFF;
            break;
        case HL:
            cpuData.regs.h = value >> 8;
            cpuData.regs.l = value & 0xFF;
            break;
        default:
            printf("Invalid combo register\n");
            break;
    }
}
void fetchOperand(){
    cpuData.destAddr = 0;
    cpuData.toMem = false;
    if (cpuData.instr == NULL){
        return;

    }
    switch(cpuData.instr->addrMode){
        case IMPL:
            break;
        case N8:
            cpuData.opSrc = memReadU(&mem, cpuData.regs.pc);
            cpuData.regs.pc++;
            break;
        case R:
            cpuData.opSrc = cpuData.instr->regSrc;
            break;
        case R_R:
            cpuData.opSrc = cpuData.instr->regSrc;
            break;
        case MEMFROMREG_R:
            cpuData.opSrc = cpuData.regs.a;
            cpuData.destAddr = getReg16(cpuData.instr->regDest);
            cpuData.toMem = true;
            break;
        case R_MEMFROMREG:
            cpuData.opSrc = memReadU(&mem, getReg16(cpuData.instr->regSrc));
            break;
        default:
            printf("This addressing mode is not supported yet\n");
            break;
    }

}
char* printState(State state){
    CpuData prevState = cpuData;
    char finalString[1000];
    fetchInstr();
    fetchOperand();
    int offset = sprintf(finalString, "Registers: A: %02X F: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X AF: %04X BC: %04X DE: %04X HL: %04X\nFlags: Z: %d N: %d H: %d C: %d\nPC: %04X  SP: %04X\n", cpuData.regs.a, cpuData.regs.f, cpuData.regs.b, cpuData.regs.c, cpuData.regs.d, cpuData.regs.e, cpuData.regs.h, cpuData.regs.l, getReg16(AF), getReg16(BC), getReg16(DE), getReg16(HL), FLAG_Z, FLAG_N, FLAG_H, FLAG_C, cpuData.regs.pc, cpuData.regs.sp);
    if (state==RUN){
        offset += sprintf(finalString + offset, "Next instruction (%02X): %s ", cpuData.opcode, getInstrName(cpuData.instr->proc));
    }
}
bool cpuStep(){
    // TODO
    printf("This place is kinda empty, isn't it?\n");
    return false;
}