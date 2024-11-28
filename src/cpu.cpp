#include <cpu.h>
#include <emu.h>
#include <memory.h>
#include <string>
#include <iostream>

Reg reg_arr[] = {A, F, B, C, D, E, H, L, AF, BC, DE, HL, SP, PC};
int size_reg_arr = sizeof(reg_arr)/sizeof(reg_arr[0]);
Reg composite_regs[] = {AF, BC, DE, HL};
int size_composite_regs = sizeof(composite_regs)/sizeof(composite_regs[0]);
Reg byte_regs[] = {A, F, B, C, D, E, H, L};
int size_byte_regs = sizeof(byte_regs)/sizeof(byte_regs[0]);
Addr_mode srcs_16bit[] = {Addr_mode::IMM16, Addr_mode::REG16, Addr_mode::REG16_PLUS_IMMe8};
int size_srcs_16bit = sizeof(srcs_16bit)/sizeof(srcs_16bit[0]);
std::map<Reg,std::pair<Reg,Reg>> reg_pairs = {
    {AF, {A, F}},
    {BC, {B, C}},
    {DE, {D, E}},
    {HL, {H, L}}

};
std::map<Cond, std::string> cond_names = {{Cond::ALWAYS, ""}, {Cond::NZ, "NZ"}, {Cond::Z, "Z"}, {Cond::NC, "NC"}, {Cond::C, "C"}};
std::map<Reg, std::string> reg_names = {{A, "A"}, {F, "F"}, {B, "B"}, {C, "C"}, {D, "D"}, {E, "E"}, {H, "H"}, {L, "L"}, {AF, "AF"}, {BC, "BC"}, {DE, "DE"}, {HL, "HL"}, {SP, "SP"}, {PC, "PC"}};
std::map<u8,Instr> Cpu::instr_map = {
    {0x00, (Instr){(Instr_args){"NOP",0x00}, &Cpu::NOP}},
    {0x01, (Instr){(Instr_args){"LD", 0x01, (Operand){Addr_mode::REG16, BC}, (Operand){Addr_mode::IMM16}}, &Cpu::LD}},
    {0x02, (Instr){(Instr_args){"LD", 0x02, (Operand){Addr_mode::MEM_REG, BC}, (Operand){Addr_mode::REG, A}}, &Cpu::LD}},

    {0x06, (Instr){(Instr_args){"LD", 0x06, (Operand){Addr_mode::REG, B}, (Operand){Addr_mode::IMM8}}, &Cpu::LD}},

    {0x08, (Instr){(Instr_args){"LD", 0x08, (Operand){Addr_mode::MEM_IMM16}, (Operand){Addr_mode::REG16, SP}}, &Cpu::LD}},

    {0x0A, (Instr){(Instr_args){"LD", 0x0A, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::MEM_REG, BC}}, &Cpu::LD}},

    {0x0E, (Instr){(Instr_args){"LD", 0x0E, (Operand){Addr_mode::REG, C}, (Operand){Addr_mode::IMM8}}, &Cpu::LD}},

    {0x26, (Instr){(Instr_args){"LD", 0x26, (Operand){Addr_mode::REG, H}, (Operand){Addr_mode::IMM8}}, &Cpu::LD}},

    {0x2E, (Instr){(Instr_args){"LD", 0x2E, (Operand){Addr_mode::REG, L}, (Operand){Addr_mode::IMM8}}, &Cpu::LD}},


    {0x3E, (Instr){(Instr_args){"LD", 0x3E, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::IMM8}}, &Cpu::LD}},

    {0xC3, (Instr){(Instr_args){"JP", 0xC3, (Operand){Addr_mode::IMM16}}, &Cpu::JP}},

    {0xF9, (Instr){(Instr_args){"LD", 0xF9, (Operand){Addr_mode::REG16, SP}, (Operand){Addr_mode::REG16, HL}}, &Cpu::LD}}

};
std::map<u8,Instr> Cpu::instr_map_prefix = {
    {0x00, (Instr){(Instr_args){"RLC B",0xCB00}, &Cpu::RLC}}
};
Cpu::Cpu(Memory& memory): mem(memory){
    this->reset();
}

void Cpu::reset(){
    this->state = RUN;
    this->regs = Reg_dict();
    this->opcode = 0;
    this->regs[PC] = 0x100;
    this->regs[SP] = 0xFFFE;
    this->regs[A] = 1;
    this->regs[F] = 0x80;
    this->regs[B] = 0x00;
    this->regs[C] = 0x13;
    this->regs[D] = 0x00;
    this->regs[E] = 0xD8;
    this->regs[H] = 0x01;
    this->regs[L] = 0x4D;
}

bool Cpu::step(){
    if(this->state == RUN){
        this->opcode = this->mem[this->regs[PC]];
        Instr curr_instr = Cpu::instr_map[this->opcode];
        this->regs[PC]++;
        run_ticks(1);
        (this->*(curr_instr.execute))(curr_instr.args);
        return true;
    }
    return false;
}
void Cpu::fetch_operand(Operand& op, bool affect){
    u16 pc_update = 0;
    int ticks_to_add = 0;
    if(!affect)
        this->mem.is_protected = false;
    switch (op.addr_mode){
        case Addr_mode::IMPL:
            break;
        case Addr_mode::IMM8: case Addr_mode::IMMe8:
            pc_update = 1;
            ticks_to_add = 1;
            op.value = this->mem[this->regs[PC]];
            break;
        case Addr_mode::IMM16:
            pc_update = 2;
            ticks_to_add = 2;
            op.value = this->mem[this->regs[PC]] | (this->mem[this->regs[PC]+1] << 8);
            break;
        case Addr_mode::REG: case Addr_mode::REG16:
            op.value = this->regs[op.reg];
            break;
        case Addr_mode::REG16_PLUS_IMMe8:
            pc_update = 1;
            ticks_to_add = 1;
            op.value = this->regs[op.reg] + static_cast<int8_t>(this->mem[this->regs[PC]]);
            break;
        case Addr_mode::MEM_REG: case Addr_mode::MEM_REG_INC: case Addr_mode::MEM_REG_DEC:
            ticks_to_add = 1;
            op.value = this->mem[this->regs[op.reg]];
            if (affect)
                this->regs[op.reg] += (op.addr_mode == Addr_mode::MEM_REG_INC) ? 1 : (op.addr_mode == Addr_mode::MEM_REG_DEC) ? -1 : 0;
            break;
        case Addr_mode::MEM_IMM16:
            pc_update = 2;
            ticks_to_add = 2;
            op.value = this->mem[this->regs[PC]] | (this->mem[this->regs[PC]+1] << 8);
            break;
        case Addr_mode::HRAM_PLUS_IMM8:
            pc_update = 1;
            ticks_to_add = 2;
            op.value = this->mem[0xFF00 + this->mem[this->regs[PC]]];
            break;
        case Addr_mode::HRAM_PLUS_C:
            ticks_to_add = 1;
            op.value = this->mem[0xFF00 + this->regs[C]];
            break;
    }
    if(affect){
        this->regs[PC] += pc_update;
        run_ticks(ticks_to_add);
    }
    else{
        this->mem.is_protected = true;
    }
}
void Cpu::write_to_operand_8bit(Operand& op, u16 value){
    value &= 0xFF;
    int ticks_to_add = 0;
    int add_to_pc = 0;
    switch(op.addr_mode){
        case Addr_mode::REG:
            this->regs[op.reg] = value;
            break;
        case Addr_mode::MEM_REG: case Addr_mode::MEM_REG_INC: case Addr_mode::MEM_REG_DEC:
            ticks_to_add = 1;
            this->mem[this->regs[op.reg]] = value;
            if(op.addr_mode == Addr_mode::MEM_REG_INC)
                this->regs[op.reg]++;
            else if(op.addr_mode == Addr_mode::MEM_REG_DEC)
                this->regs[op.reg]--;
            break;
        case Addr_mode::MEM_IMM16:
            ticks_to_add = 3;
            add_to_pc = 2;
            this->mem[this->mem[this->regs[PC]] | (this->mem[this->regs[PC]+1] << 8)] = value;
            break;
        case Addr_mode::HRAM_PLUS_IMM8:
            ticks_to_add = 2;
            add_to_pc = 1;
            this->mem[0xFF00 + this->mem[this->regs[PC]]] = value;
            break;
        case Addr_mode::HRAM_PLUS_C:
            ticks_to_add = 1;
            this->mem[0xFF00 + this->regs[C]] = value;
            break;
        default:
            std::cout<<"Erm... what the sigma? (write8bit)"<<std::endl;
            break;
    }
    this->regs[PC] += add_to_pc;
    run_ticks(ticks_to_add);
}
void Cpu::write_to_operand_16bit(Operand& op, u16 value){
    int ticks_to_add = 0;
    int add_to_pc = 0;
    switch(op.addr_mode){
        case Addr_mode::REG16:
            this->regs[op.reg] = value;
            break;
        case Addr_mode::MEM_IMM16:
            ticks_to_add = 4;
            add_to_pc = 2;
            this->mem[this->mem[this->regs[PC]] | (this->mem[this->regs[PC]+1] << 8)] = value & 0xFF;
            this->mem[(this->mem[this->regs[PC]] | (this->mem[this->regs[PC]+1] << 8)) + 1] = value >> 8;
            break;
        case Addr_mode::MEM_REG:
            ticks_to_add = 2;
            this->mem[this->regs[op.reg]] = value & 0xFF;
            this->mem[this->regs[op.reg] + 1] = value >> 8;
            break;
        default:
            std::cout<<"Erm... what the sigma? (write16bit)"<<std::endl;
            break;
    }
    this->regs[PC] += add_to_pc;
    run_ticks(ticks_to_add);
}
void Cpu::write_to_operand(Operand&op, u16 value, Addr_mode src_addr){
    if (std::find(srcs_16bit, srcs_16bit + size_srcs_16bit, src_addr) == srcs_16bit + size_srcs_16bit){
        write_to_operand_8bit(op, value);
    }
    else{ //Assuming there is no broken instruction that t
        write_to_operand_16bit(op, value);
    }
}
std::string Cpu::operand_toString(Operand op){
    std::string str = "";
    switch(op.addr_mode){
        case Addr_mode::IMPL:
            break;
        case Addr_mode::IMM8: case Addr_mode::IMMe8:
            this->fetch_operand(op, false);
            str += "$"+(op.addr_mode == Addr_mode::IMM8 ? numToHexString(op.value, 2) : numToHexString(static_cast<int8_t>(op.value), 2));
            break;
        case Addr_mode::IMM16:
            this->fetch_operand(op, false);
            str += "$"+numToHexString(op.value, 4);
            break;  
        case Addr_mode::REG: case Addr_mode::REG16:
            str += reg_names[op.reg];
            break;
        case Addr_mode::REG16_PLUS_IMMe8:
            this->fetch_operand(op, false);
            str += reg_names[op.reg] + " + $" + numToHexString(static_cast<int8_t>(op.value), 2);
            break;
        case Addr_mode::MEM_REG: case Addr_mode::MEM_REG_INC: case Addr_mode::MEM_REG_DEC:
            str += "[" + reg_names[op.reg] + (op.addr_mode == Addr_mode::MEM_REG_INC ? "+" : op.addr_mode == Addr_mode::MEM_REG_DEC ? "-" : "") + "]";
            break;
        case Addr_mode::MEM_IMM16:
            this->fetch_operand(op, false);
            str += "[" + numToHexString(op.value, 4) + "]";
            break;
        case Addr_mode::HRAM_PLUS_IMM8:
            this->fetch_operand(op, false);
            str += "[0xFF00 + $" + numToHexString(op.value, 2) + "]";
            break;
        case Addr_mode::HRAM_PLUS_C:
            str += "[0xFF00 + " + reg_names[C] + "]";
            break;
    }
    return str;
}
std::string Cpu::instr_toString(Instr instr){
    this->regs[PC]++;
    Instr_args args = instr.args;
    std::string str = args.name + " ";
    if(args.cond != Cond::ALWAYS)
        str += cond_names[args.cond] + " ";
    str += this->operand_toString(args.dest);
    if(args.src.addr_mode != Addr_mode::IMPL){
        str += ", " + this->operand_toString(args.src);
    }
    this->regs[PC]--;
    return str;
}
std::string Cpu::toString(){
    std::string str = "Cpu state: ";
    switch(this->state){
        case RUN:
            str += "RUN";
            break;
        case STOP:
            str += "STOP";
            break;
        case HALT:
            str += "HALT";
            break;
        case PAUSE:
            str += "PAUSE";
            break;
        case QUIT:
            str += "QUIT";
            break;
    }
    str += "\nRegisters:\n";
    for (int i=0; i<(int)(sizeof(reg_arr)/sizeof(reg_arr[0])-2); i++){
        str += reg_names[reg_arr[i]] + ": " + numToHexString(this->regs[reg_arr[i]], i<8 ? 2 : 4) + " ";
    }
    str += "\nPC: " + numToHexString(this->regs[PC],4) + " SP: " + numToHexString(this->regs[SP],4);
    str+="\n\nInstruction: ";
    if (this->mem.readX(this->regs[PC]) != 0xCB)
        str += this->instr_toString(Cpu::instr_map[this->mem.readX(this->regs[PC])]);
    else{
        this->regs[PC]++;
        str += this->instr_toString(Cpu::instr_map_prefix[this->mem.readX(this->regs[PC])]);
        this->regs[PC]--;
    }
    return str;
}
void Cpu::noImpl(Instr_args args){
    std::cout << args.name << " is not implemented yet" << std::endl;
    this->state = QUIT;
    return;
}
void Cpu::NOP(Instr_args args){
    return;
}
void Cpu::LD(Instr_args args){
    fetch_operand(args.src);
    write_to_operand(args.dest, args.src.value, args.src.addr_mode);
}
void Cpu::INC(Instr_args args){
    this->noImpl(args);
}
void Cpu::DEC(Instr_args args){
    this->noImpl(args);
}
void Cpu::RLC(Instr_args args){
    this->noImpl(args);
}
void Cpu::ADD(Instr_args args){
    this->noImpl(args);
}
void Cpu::JP(Instr_args args){
    fetch_operand(args.dest);
    this->regs[PC] = args.dest.value;
}