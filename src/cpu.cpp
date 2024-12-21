#include <cpu.h>
#include <emu.h>
#include <memory.h>
#include <string>
#include <iostream>
#include <utils.h>

Flag flag_arr[] = {Flag::Z, Flag::N, Flag::H, Flag::C};
int size_flag_arr = sizeof(flag_arr)/sizeof(flag_arr[0]);
Reg reg_arr[] = {A, F, B, C, D, E, H, L, AF, BC, DE, HL, SP, PC};
int size_reg_arr = sizeof(reg_arr)/sizeof(reg_arr[0]);
Reg composite_regs[] = {AF, BC, DE, HL};
int size_composite_regs = sizeof(composite_regs)/sizeof(composite_regs[0]);
Reg byte_regs[] = {A, F, B, C, D, E, H, L};
int size_byte_regs = sizeof(byte_regs)/sizeof(byte_regs[0]);
Addr_mode srcs_16bit[] = {Addr_mode::IMM16, Addr_mode::REG16, Addr_mode::REG16_PLUS_IMMe8, Addr_mode:: MEM16_REG};
int size_srcs_16bit = sizeof(srcs_16bit)/sizeof(srcs_16bit[0]);
std::map<Reg,std::pair<Reg,Reg>> reg_pairs = {
    {AF, {A, F}},
    {BC, {B, C}},
    {DE, {D, E}},
    {HL, {H, L}}

};
std::map<Cpu_State,std::string> cpu_state_names = {{Cpu_State::RUNNING, "RUNNING"}, {Cpu_State::STOPPED, "STOPPED"}, {Cpu_State::HALTED, "HALTED"}, {Cpu_State::QUIT, "QUIT"}, {Cpu_State::PAUSED, "PAUSED"}};
std::map<Flag,u16> flag_despl = {{Flag::Z, 7}, {Flag::N, 6}, {Flag::H, 5}, {Flag::C, 4}};
std::map<Flag,std::string> flag_names = {{Flag::Z,"Z"}, {Flag::N, "N"}, {Flag::H,"H"}, {Flag::C, "C"}};
std::map<Cond, std::string> cond_names = {{Cond::ALWAYS, ""}, {Cond::NZ, "NZ"}, {Cond::Z, "Z"}, {Cond::NC, "NC"}, {Cond::C, "C"}};
std::map<Reg, std::string> reg_names = {{A, "A"}, {F, "F"}, {B, "B"}, {C, "C"}, {D, "D"}, {E, "E"}, {H, "H"}, {L, "L"}, {AF, "AF"}, {BC, "BC"}, {DE, "DE"}, {HL, "HL"}, {SP, "SP"}, {PC, "PC"}};
std::map<u8,Instr> Cpu::instr_map = {
    {0x00, (Instr){(Instr_args){"NOP",0x00}, &Cpu::NOP}}, //OK
    {0x01, (Instr){(Instr_args){"LD", 0x01, (Operand){Addr_mode::REG16, BC}, (Operand){Addr_mode::IMM16}}, &Cpu::LD}},
    {0x02, (Instr){(Instr_args){"LD", 0x02, (Operand){Addr_mode::MEM_REG, BC}, (Operand){Addr_mode::REG, A}}, &Cpu::LD}}, //OK
    {0x03, (Instr){(Instr_args){"INC", 0x03, (Operand){Addr_mode::REG16, BC}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::ADD}},
    {0x04, (Instr){(Instr_args){"INC", 0x04, (Operand){Addr_mode::REG, B}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::ADD}},
    {0x05, (Instr){(Instr_args){"DEC", 0x05, (Operand){Addr_mode::REG, B}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::SUB}},
    {0x06, (Instr){(Instr_args){"LD", 0x06, (Operand){Addr_mode::REG, B}, (Operand){Addr_mode::IMM8}}, &Cpu::LD}}, //OK
    {0x07, (Instr){(Instr_args){"RLCA", 0x07, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::IMPL}, Cond::ALWAYS, 2}, &Cpu::ROT}},
    {0x08, (Instr){(Instr_args){"LD", 0x08, (Operand){Addr_mode::MEM_IMM16}, (Operand){Addr_mode::REG16, SP}}, &Cpu::LD}}, //OK
    {0x09, (Instr){(Instr_args){"ADD", 0x09, (Operand){Addr_mode::REG16, HL}, (Operand){Addr_mode::REG16, BC}}, &Cpu::ADD}},
    {0x0A, (Instr){(Instr_args){"LD", 0x0A, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::MEM_REG, BC}}, &Cpu::LD}},
    {0x0B, (Instr){(Instr_args){"DEC", 0x0B, (Operand){Addr_mode::REG16, BC}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::SUB}},
    {0x0C, (Instr){(Instr_args){"INC", 0x0C, (Operand){Addr_mode::REG, C}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::ADD}},
    {0x0D, (Instr){(Instr_args){"DEC", 0x0D, (Operand){Addr_mode::REG, C}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::SUB}},
    {0x0E, (Instr){(Instr_args){"LD", 0x0E, (Operand){Addr_mode::REG, C}, (Operand){Addr_mode::IMM8}}, &Cpu::LD}}, //OK
    {0x0F, (Instr){(Instr_args){"RRCA", 0x0F, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::IMPL}, Cond::ALWAYS, 6}, &Cpu::ROT}},
    {0x10, (Instr){(Instr_args){"STOP", 0x10, (Operand){Addr_mode::IMPL, NO_REG, 1}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 4}, &Cpu::STOP}},
    {0x11, (Instr){(Instr_args){"LD", 0x11, (Operand){Addr_mode::REG16, DE}, (Operand){Addr_mode::IMM16}}, &Cpu::LD}},
    {0x12, (Instr){(Instr_args){"LD", 0x12, (Operand){Addr_mode::MEM_REG, DE}, (Operand){Addr_mode::REG, A}}, &Cpu::LD}}, //OK
    {0x13, (Instr){(Instr_args){"INC", 0x13, (Operand){Addr_mode::REG16, DE}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::ADD}},
    {0x14, (Instr){(Instr_args){"INC", 0x14, (Operand){Addr_mode::REG, D}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::ADD}},
    {0x15, (Instr){(Instr_args){"DEC", 0x15, (Operand){Addr_mode::REG, D}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::SUB}},
    {0x16, (Instr){(Instr_args){"LD", 0x16, (Operand){Addr_mode::REG, D}, (Operand){Addr_mode::IMM8}}, &Cpu::LD}}, //OK
    {0x17, (Instr){(Instr_args){"RLA", 0x17, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::IMPL}, Cond::ALWAYS, 0}, &Cpu::ROT}},
    {0x18, (Instr){(Instr_args){"JR", 0x18, (Operand){Addr_mode::IMMe8}}, &Cpu::JP}},
    {0x19, (Instr){(Instr_args){"ADD", 0x19, (Operand){Addr_mode::REG16, HL}, (Operand){Addr_mode::REG16, DE}}, &Cpu::ADD}},
    {0x1A, (Instr){(Instr_args){"LD", 0x1A, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::MEM_REG, DE}}, &Cpu::LD}}, //OK
    {0x1B, (Instr){(Instr_args){"DEC", 0x1B, (Operand){Addr_mode::REG16, DE}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::SUB}},
    {0x1C, (Instr){(Instr_args){"INC", 0x1C, (Operand){Addr_mode::REG, E}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::ADD}},
    {0x1D, (Instr){(Instr_args){"DEC", 0x1D, (Operand){Addr_mode::REG, E}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::SUB}},
    {0x1E, (Instr){(Instr_args){"LD", 0x1E, (Operand){Addr_mode::REG, E}, (Operand){Addr_mode::IMM8}}, &Cpu::LD}}, //OK
    {0x1F, (Instr){(Instr_args){"RRA", 0x1F, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::IMPL}, Cond::ALWAYS, 4}, &Cpu::ROT}},
    {0x20, (Instr){(Instr_args){"JR", 0x20, (Operand){Addr_mode::IMMe8}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::NZ}, &Cpu::JP}},
    {0x21, (Instr){(Instr_args){"LD", 0x21, (Operand){Addr_mode::REG16, HL}, (Operand){Addr_mode::IMM16}}, &Cpu::LD}},
    {0x22, (Instr){(Instr_args){"LD", 0x22, (Operand){Addr_mode::MEM_REG_INC, HL, 0}, (Operand){Addr_mode::REG, A}}, &Cpu::LD}}, //OK
    {0x23, (Instr){(Instr_args){"INC", 0x23, (Operand){Addr_mode::REG16, HL}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::ADD}},
    {0x24, (Instr){(Instr_args){"INC", 0x24, (Operand){Addr_mode::REG, H}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::ADD}},
    {0x25, (Instr){(Instr_args){"DEC", 0x25, (Operand){Addr_mode::REG, H}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::SUB}},
    {0x26, (Instr){(Instr_args){"LD", 0x26, (Operand){Addr_mode::REG, H}, (Operand){Addr_mode::IMM8}}, &Cpu::LD}}, //OK
    {0x27, (Instr){(Instr_args){"DAA", 0x27}, &Cpu::DAA}},
    {0x28, (Instr){(Instr_args){"JR", 0x28, (Operand){Addr_mode::IMMe8}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::Z}, &Cpu::JP}},

    {0x29, (Instr){(Instr_args){"ADD", 0x29, (Operand){Addr_mode::REG16, HL}, (Operand){Addr_mode::REG16, HL}}, &Cpu::ADD}},
    {0x2A, (Instr){(Instr_args){"LD", 0x2A, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::MEM_REG_INC, HL, 0}}, &Cpu::LD}}, //OK
    {0x2B, (Instr){(Instr_args){"DEC", 0x2B, (Operand){Addr_mode::REG16, HL}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::SUB}},
    {0x2C, (Instr){(Instr_args){"INC", 0x2C, (Operand){Addr_mode::REG, L}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::ADD}},
    {0x2D, (Instr){(Instr_args){"DEC", 0x2D, (Operand){Addr_mode::REG, L}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::SUB}},
    {0x2E, (Instr){(Instr_args){"LD", 0x2E, (Operand){Addr_mode::REG, L}, (Operand){Addr_mode::IMM8}}, &Cpu::LD}}, //OK
    {0x2F, (Instr){(Instr_args){"CPL", 0x2F}, &Cpu::CPL}},
    {0x30, (Instr){(Instr_args){"JR", 0x30, (Operand){Addr_mode::IMMe8}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::NC}, &Cpu::JP}},

    {0x32, (Instr){(Instr_args){"LD", 0x32, (Operand){Addr_mode::MEM_REG_DEC, HL, 0}, (Operand){Addr_mode::REG, A}}, &Cpu::LD}},
    {0x33, (Instr){(Instr_args){"INC", 0x33, (Operand){Addr_mode::REG16, SP}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::ADD}},
    {0x34, (Instr){(Instr_args){"INC", 0x34, (Operand){Addr_mode::MEM_REG, HL}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::ADD}},
    {0x35, (Instr){(Instr_args){"DEC", 0x35, (Operand){Addr_mode::MEM_REG, HL}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::SUB}},
    {0x36, (Instr){(Instr_args){"LD", 0x36, (Operand){Addr_mode::MEM_REG, HL}, (Operand){Addr_mode::IMM8}}, &Cpu::LD}}, //OK

    {0x38, (Instr){(Instr_args){"JR", 0x38, (Operand){Addr_mode::IMMe8}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::C}, &Cpu::JP}},
    {0x39, (Instr){(Instr_args){"ADD", 0x39, (Operand){Addr_mode::REG16, HL}, (Operand){Addr_mode::REG16, SP}}, &Cpu::ADD}},
    {0x3A, (Instr){(Instr_args){"LD", 0x3A, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::MEM_REG_DEC, HL, 0}}, &Cpu::LD}},
    {0x3B, (Instr){(Instr_args){"DEC", 0x3B, (Operand){Addr_mode::REG16, SP}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::SUB}},
    {0x3C, (Instr){(Instr_args){"INC", 0x3C, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::ADD}},
    {0x3D, (Instr){(Instr_args){"DEC", 0x3D, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::IMPL, NO_REG, 1}, Cond::ALWAYS, 2}, &Cpu::SUB}},
    {0x3E, (Instr){(Instr_args){"LD", 0x3E, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::IMM8}}, &Cpu::LD}}, //OK

    {0x40, (Instr){(Instr_args){"LD", 0x40, (Operand){Addr_mode::REG, B}, (Operand){Addr_mode::REG, B}}, &Cpu::LD}},
    {0x41, (Instr){(Instr_args){"LD", 0x41, (Operand){Addr_mode::REG, B}, (Operand){Addr_mode::REG, C}}, &Cpu::LD}},
    {0x42, (Instr){(Instr_args){"LD", 0x42, (Operand){Addr_mode::REG, B}, (Operand){Addr_mode::REG, D}}, &Cpu::LD}},
    {0x43, (Instr){(Instr_args){"LD", 0x43, (Operand){Addr_mode::REG, B}, (Operand){Addr_mode::REG, E}}, &Cpu::LD}},
    {0x44, (Instr){(Instr_args){"LD", 0x44, (Operand){Addr_mode::REG, B}, (Operand){Addr_mode::REG, H}}, &Cpu::LD}},
    {0x45, (Instr){(Instr_args){"LD", 0x45, (Operand){Addr_mode::REG, B}, (Operand){Addr_mode::REG, L}}, &Cpu::LD}},
    {0x46, (Instr){(Instr_args){"LD", 0x46, (Operand){Addr_mode::REG, B}, (Operand){Addr_mode::MEM_REG, HL}}, &Cpu::LD}},
    {0x47, (Instr){(Instr_args){"LD", 0x47, (Operand){Addr_mode::REG, B}, (Operand){Addr_mode::REG, A}}, &Cpu::LD}},
    {0x48, (Instr){(Instr_args){"LD", 0x48, (Operand){Addr_mode::REG, C}, (Operand){Addr_mode::REG, B}}, &Cpu::LD}},
    {0x49, (Instr){(Instr_args){"LD", 0x49, (Operand){Addr_mode::REG, C}, (Operand){Addr_mode::REG, C}}, &Cpu::LD}},
    {0x4A, (Instr){(Instr_args){"LD", 0x4A, (Operand){Addr_mode::REG, C}, (Operand){Addr_mode::REG, D}}, &Cpu::LD}},
    {0x4B, (Instr){(Instr_args){"LD", 0x4B, (Operand){Addr_mode::REG, C}, (Operand){Addr_mode::REG, E}}, &Cpu::LD}},
    {0x4C, (Instr){(Instr_args){"LD", 0x4C, (Operand){Addr_mode::REG, C}, (Operand){Addr_mode::REG, H}}, &Cpu::LD}},
    {0x4D, (Instr){(Instr_args){"LD", 0x4D, (Operand){Addr_mode::REG, C}, (Operand){Addr_mode::REG, L}}, &Cpu::LD}},
    {0x4E, (Instr){(Instr_args){"LD", 0x4E, (Operand){Addr_mode::REG, C}, (Operand){Addr_mode::MEM_REG, HL}}, &Cpu::LD}},
    {0x4F, (Instr){(Instr_args){"LD", 0x4F, (Operand){Addr_mode::REG, C}, (Operand){Addr_mode::REG, A}}, &Cpu::LD}},
    {0x50, (Instr){(Instr_args){"LD", 0x50, (Operand){Addr_mode::REG, D}, (Operand){Addr_mode::REG, B}}, &Cpu::LD}},
    {0x51, (Instr){(Instr_args){"LD", 0x51, (Operand){Addr_mode::REG, D}, (Operand){Addr_mode::REG, C}}, &Cpu::LD}},
    {0x52, (Instr){(Instr_args){"LD", 0x52, (Operand){Addr_mode::REG, D}, (Operand){Addr_mode::REG, D}}, &Cpu::LD}},
    {0x53, (Instr){(Instr_args){"LD", 0x53, (Operand){Addr_mode::REG, D}, (Operand){Addr_mode::REG, E}}, &Cpu::LD}},
    {0x54, (Instr){(Instr_args){"LD", 0x54, (Operand){Addr_mode::REG, D}, (Operand){Addr_mode::REG, H}}, &Cpu::LD}},
    {0x55, (Instr){(Instr_args){"LD", 0x55, (Operand){Addr_mode::REG, D}, (Operand){Addr_mode::REG, L}}, &Cpu::LD}},
    {0x56, (Instr){(Instr_args){"LD", 0x56, (Operand){Addr_mode::REG, D}, (Operand){Addr_mode::MEM_REG, HL}}, &Cpu::LD}},
    {0x57, (Instr){(Instr_args){"LD", 0x57, (Operand){Addr_mode::REG, D}, (Operand){Addr_mode::REG, A}}, &Cpu::LD}},
    {0x58, (Instr){(Instr_args){"LD", 0x58, (Operand){Addr_mode::REG, E}, (Operand){Addr_mode::REG, B}}, &Cpu::LD}},
    {0x59, (Instr){(Instr_args){"LD", 0x59, (Operand){Addr_mode::REG, E}, (Operand){Addr_mode::REG, C}}, &Cpu::LD}},
    {0x5A, (Instr){(Instr_args){"LD", 0x5A, (Operand){Addr_mode::REG, E}, (Operand){Addr_mode::REG, D}}, &Cpu::LD}},
    {0x5B, (Instr){(Instr_args){"LD", 0x5B, (Operand){Addr_mode::REG, E}, (Operand){Addr_mode::REG, E}}, &Cpu::LD}},
    {0x5C, (Instr){(Instr_args){"LD", 0x5C, (Operand){Addr_mode::REG, E}, (Operand){Addr_mode::REG, H}}, &Cpu::LD}},
    {0x5D, (Instr){(Instr_args){"LD", 0x5D, (Operand){Addr_mode::REG, E}, (Operand){Addr_mode::REG, L}}, &Cpu::LD}},
    {0x5E, (Instr){(Instr_args){"LD", 0x5E, (Operand){Addr_mode::REG, E}, (Operand){Addr_mode::MEM_REG, HL}}, &Cpu::LD}},
    {0x5F, (Instr){(Instr_args){"LD", 0x5F, (Operand){Addr_mode::REG, E}, (Operand){Addr_mode::REG, A}}, &Cpu::LD}},
    {0x60, (Instr){(Instr_args){"LD", 0x60, (Operand){Addr_mode::REG, H}, (Operand){Addr_mode::REG, B}}, &Cpu::LD}},
    {0x61, (Instr){(Instr_args){"LD", 0x61, (Operand){Addr_mode::REG, H}, (Operand){Addr_mode::REG, C}}, &Cpu::LD}},
    {0x62, (Instr){(Instr_args){"LD", 0x62, (Operand){Addr_mode::REG, H}, (Operand){Addr_mode::REG, D}}, &Cpu::LD}},
    {0x63, (Instr){(Instr_args){"LD", 0x63, (Operand){Addr_mode::REG, H}, (Operand){Addr_mode::REG, E}}, &Cpu::LD}},
    {0x64, (Instr){(Instr_args){"LD", 0x64, (Operand){Addr_mode::REG, H}, (Operand){Addr_mode::REG, H}}, &Cpu::LD}},
    {0x65, (Instr){(Instr_args){"LD", 0x65, (Operand){Addr_mode::REG, H}, (Operand){Addr_mode::REG, L}}, &Cpu::LD}},
    {0x66, (Instr){(Instr_args){"LD", 0x66, (Operand){Addr_mode::REG, H}, (Operand){Addr_mode::MEM_REG, HL}}, &Cpu::LD}},
    {0x67, (Instr){(Instr_args){"LD", 0x67, (Operand){Addr_mode::REG, H}, (Operand){Addr_mode::REG, A}}, &Cpu::LD}},
    {0x68, (Instr){(Instr_args){"LD", 0x68, (Operand){Addr_mode::REG, L}, (Operand){Addr_mode::REG, B}}, &Cpu::LD}},
    {0x69, (Instr){(Instr_args){"LD", 0x69, (Operand){Addr_mode::REG, L}, (Operand){Addr_mode::REG, C}}, &Cpu::LD}},
    {0x6A, (Instr){(Instr_args){"LD", 0x6A, (Operand){Addr_mode::REG, L}, (Operand){Addr_mode::REG, D}}, &Cpu::LD}},
    {0x6B, (Instr){(Instr_args){"LD", 0x6B, (Operand){Addr_mode::REG, L}, (Operand){Addr_mode::REG, E}}, &Cpu::LD}},
    {0x6C, (Instr){(Instr_args){"LD", 0x6C, (Operand){Addr_mode::REG, L}, (Operand){Addr_mode::REG, H}}, &Cpu::LD}},
    {0x6D, (Instr){(Instr_args){"LD", 0x6D, (Operand){Addr_mode::REG, L}, (Operand){Addr_mode::REG, L}}, &Cpu::LD}},
    {0x6E, (Instr){(Instr_args){"LD", 0x6E, (Operand){Addr_mode::REG, L}, (Operand){Addr_mode::MEM_REG, HL}}, &Cpu::LD}},
    {0x6F, (Instr){(Instr_args){"LD", 0x6F, (Operand){Addr_mode::REG, L}, (Operand){Addr_mode::REG, A}}, &Cpu::LD}},
    {0x70, (Instr){(Instr_args){"LD", 0x70, (Operand){Addr_mode::MEM_REG, HL}, (Operand){Addr_mode::REG, B}}, &Cpu::LD}},
    {0x71, (Instr){(Instr_args){"LD", 0x71, (Operand){Addr_mode::MEM_REG, HL}, (Operand){Addr_mode::REG, C}}, &Cpu::LD}},
    {0x72, (Instr){(Instr_args){"LD", 0x72, (Operand){Addr_mode::MEM_REG, HL}, (Operand){Addr_mode::REG, D}}, &Cpu::LD}},
    {0x73, (Instr){(Instr_args){"LD", 0x73, (Operand){Addr_mode::MEM_REG, HL}, (Operand){Addr_mode::REG, E}}, &Cpu::LD}},
    {0x74, (Instr){(Instr_args){"LD", 0x74, (Operand){Addr_mode::MEM_REG, HL}, (Operand){Addr_mode::REG, H}}, &Cpu::LD}},
    {0x75, (Instr){(Instr_args){"LD", 0x75, (Operand){Addr_mode::MEM_REG, HL}, (Operand){Addr_mode::REG, L}}, &Cpu::LD}},
    {0x76, (Instr){(Instr_args){"HALT", 0x76}, &Cpu::HALT}},
    {0x77, (Instr){(Instr_args){"LD", 0x77, (Operand){Addr_mode::MEM_REG, HL}, (Operand){Addr_mode::REG, A}}, &Cpu::LD}},
    {0x78, (Instr){(Instr_args){"LD", 0x78, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, B}}, &Cpu::LD}},
    {0x79, (Instr){(Instr_args){"LD", 0x79, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, C}}, &Cpu::LD}},
    {0x7A, (Instr){(Instr_args){"LD", 0x7A, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, D}}, &Cpu::LD}},
    {0x7B, (Instr){(Instr_args){"LD", 0x7B, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, E}}, &Cpu::LD}},
    {0x7C, (Instr){(Instr_args){"LD", 0x7C, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, H}}, &Cpu::LD}},
    {0x7D, (Instr){(Instr_args){"LD", 0x7D, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, L}}, &Cpu::LD}},
    {0x7E, (Instr){(Instr_args){"LD", 0x7E, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::MEM_REG, HL}}, &Cpu::LD}},
    {0x7F, (Instr){(Instr_args){"LD", 0x7F, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, A}}, &Cpu::LD}},
    {0x80, (Instr){(Instr_args){"ADD", 0x80, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, B}}, &Cpu::ADD}},
    {0x81, (Instr){(Instr_args){"ADD", 0x81, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, C}}, &Cpu::ADD}},
    {0x82, (Instr){(Instr_args){"ADD", 0x82, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, D}}, &Cpu::ADD}},
    {0x83, (Instr){(Instr_args){"ADD", 0x83, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, E}}, &Cpu::ADD}},
    {0x84, (Instr){(Instr_args){"ADD", 0x84, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, H}}, &Cpu::ADD}},
    {0x85, (Instr){(Instr_args){"ADD", 0x85, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, L}}, &Cpu::ADD}},
    {0x86, (Instr){(Instr_args){"ADD", 0x86, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::MEM_REG, HL}}, &Cpu::ADD}},
    {0x87, (Instr){(Instr_args){"ADD", 0x87, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, A}}, &Cpu::ADD}},
    {0x88, (Instr){(Instr_args){"ADC", 0x88, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, B}, Cond::ALWAYS, 1}, &Cpu::ADD}},
    {0x89, (Instr){(Instr_args){"ADC", 0x89, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, C}, Cond::ALWAYS, 1}, &Cpu::ADD}},
    {0x8A, (Instr){(Instr_args){"ADC", 0x8A, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, D}, Cond::ALWAYS, 1}, &Cpu::ADD}},
    {0x8B, (Instr){(Instr_args){"ADC", 0x8B, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, E}, Cond::ALWAYS, 1}, &Cpu::ADD}},
    {0x8C, (Instr){(Instr_args){"ADC", 0x8C, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, H}, Cond::ALWAYS, 1}, &Cpu::ADD}},
    {0x8D, (Instr){(Instr_args){"ADC", 0x8D, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, L}, Cond::ALWAYS, 1}, &Cpu::ADD}},
    {0x8E, (Instr){(Instr_args){"ADC", 0x8E, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::MEM_REG, HL}, Cond::ALWAYS, 1}, &Cpu::ADD}},
    {0x8F, (Instr){(Instr_args){"ADC", 0x8F, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, A}, Cond::ALWAYS, 1}, &Cpu::ADD}},
    {0x90, (Instr){(Instr_args){"SUB", 0x90, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, B}}, &Cpu::SUB}},
    {0x91, (Instr){(Instr_args){"SUB", 0x91, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, C}}, &Cpu::SUB}},
    {0x92, (Instr){(Instr_args){"SUB", 0x92, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, D}}, &Cpu::SUB}},
    {0x93, (Instr){(Instr_args){"SUB", 0x93, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, E}}, &Cpu::SUB}},
    {0x94, (Instr){(Instr_args){"SUB", 0x94, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, H}}, &Cpu::SUB}},
    {0x95, (Instr){(Instr_args){"SUB", 0x95, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, L}}, &Cpu::SUB}},
    {0x96, (Instr){(Instr_args){"SUB", 0x96, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::MEM_REG, HL}}, &Cpu::SUB}},
    {0x97, (Instr){(Instr_args){"SUB", 0x97, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, A}}, &Cpu::SUB}},
    {0x98, (Instr){(Instr_args){"SBC", 0x98, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, B}, Cond::ALWAYS, 1}, &Cpu::SUB}},
    {0x99, (Instr){(Instr_args){"SBC", 0x99, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, C}, Cond::ALWAYS, 1}, &Cpu::SUB}},
    {0x9A, (Instr){(Instr_args){"SBC", 0x9A, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, D}, Cond::ALWAYS, 1}, &Cpu::SUB}},
    {0x9B, (Instr){(Instr_args){"SBC", 0x9B, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, E}, Cond::ALWAYS, 1}, &Cpu::SUB}},
    {0x9C, (Instr){(Instr_args){"SBC", 0x9C, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, H}, Cond::ALWAYS, 1}, &Cpu::SUB}},
    {0x9D, (Instr){(Instr_args){"SBC", 0x9D, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, L}, Cond::ALWAYS, 1}, &Cpu::SUB}},
    {0x9E, (Instr){(Instr_args){"SBC", 0x9E, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::MEM_REG, HL}, Cond::ALWAYS, 1}, &Cpu::SUB}},
    {0x9F, (Instr){(Instr_args){"SBC", 0x9F, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, A}, Cond::ALWAYS, 1}, &Cpu::SUB}},
    {0xA0, (Instr){(Instr_args){"AND", 0xA0, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, B}}, &Cpu::AND}},
    {0xA1, (Instr){(Instr_args){"AND", 0xA1, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, C}}, &Cpu::AND}},
    {0xA2, (Instr){(Instr_args){"AND", 0xA2, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, D}}, &Cpu::AND}},
    {0xA3, (Instr){(Instr_args){"AND", 0xA3, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, E}}, &Cpu::AND}},
    {0xA4, (Instr){(Instr_args){"AND", 0xA4, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, H}}, &Cpu::AND}},
    {0xA5, (Instr){(Instr_args){"AND", 0xA5, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, L}}, &Cpu::AND}},
    {0xA6, (Instr){(Instr_args){"AND", 0xA6, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::MEM_REG, HL}}, &Cpu::AND}},
    {0xA7, (Instr){(Instr_args){"AND", 0xA7, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, A}}, &Cpu::AND}},
    {0xA8, (Instr){(Instr_args){"XOR", 0xA8, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, B}}, &Cpu::XOR}},
    {0xA9, (Instr){(Instr_args){"XOR", 0xA9, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, C}}, &Cpu::XOR}},
    {0xAA, (Instr){(Instr_args){"XOR", 0xAA, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, D}}, &Cpu::XOR}},
    {0xAB, (Instr){(Instr_args){"XOR", 0xAB, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, E}}, &Cpu::XOR}},
    {0xAC, (Instr){(Instr_args){"XOR", 0xAC, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, H}}, &Cpu::XOR}},
    {0xAD, (Instr){(Instr_args){"XOR", 0xAD, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, L}}, &Cpu::XOR}},
    {0xAE, (Instr){(Instr_args){"XOR", 0xAE, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::MEM_REG, HL}}, &Cpu::XOR}},
    {0xAF, (Instr){(Instr_args){"XOR", 0xAF, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, A}}, &Cpu::XOR}},
    {0xB0, (Instr){(Instr_args){"OR", 0xB0, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, B}}, &Cpu::OR}},
    {0xB1, (Instr){(Instr_args){"OR", 0xB1, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, C}}, &Cpu::OR}},
    {0xB2, (Instr){(Instr_args){"OR", 0xB2, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, D}}, &Cpu::OR}},
    {0xB3, (Instr){(Instr_args){"OR", 0xB3, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, E}}, &Cpu::OR}},
    {0xB4, (Instr){(Instr_args){"OR", 0xB4, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, H}}, &Cpu::OR}},
    {0xB5, (Instr){(Instr_args){"OR", 0xB5, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, L}}, &Cpu::OR}},
    {0xB6, (Instr){(Instr_args){"OR", 0xB6, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::MEM_REG, HL}}, &Cpu::OR}},
    {0xB7, (Instr){(Instr_args){"OR", 0xB7, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, A}}, &Cpu::OR}},
    {0xB8, (Instr){(Instr_args){"CP", 0xB8, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, B}}, &Cpu::CP}},
    {0xB9, (Instr){(Instr_args){"CP", 0xB9, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, C}}, &Cpu::CP}},
    {0xBA, (Instr){(Instr_args){"CP", 0xBA, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, D}}, &Cpu::CP}},
    {0xBB, (Instr){(Instr_args){"CP", 0xBB, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, E}}, &Cpu::CP}},
    {0xBC, (Instr){(Instr_args){"CP", 0xBC, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, H}}, &Cpu::CP}},
    {0xBD, (Instr){(Instr_args){"CP", 0xBD, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, L}}, &Cpu::CP}},
    {0xBE, (Instr){(Instr_args){"CP", 0xBE, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::MEM_REG, HL}}, &Cpu::CP}},
    {0xBF, (Instr){(Instr_args){"CP", 0xBF, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::REG, A}}, &Cpu::CP}},
    {0xC0, (Instr){(Instr_args){"RET", 0xC0, (Operand){Addr_mode::MEM16_REG, SP}, (Operand){Addr_mode::IMPL}, Cond::NZ, 2}, &Cpu::JP}},

    {0xC2, (Instr){(Instr_args){"JP", 0xC2, (Operand){Addr_mode::IMM16}, (Operand){Addr_mode::IMPL, NO_REG, 0}, Cond::NZ}, &Cpu::JP}}, //OK
    {0xC3, (Instr){(Instr_args){"JP", 0xC3, (Operand){Addr_mode::IMM16}}, &Cpu::JP}}, //OK
    {0xC4, (Instr){(Instr_args){"CALL", 0xC4, (Operand){Addr_mode::IMM16}, (Operand){Addr_mode::IMPL}, Cond::NZ, 1}, &Cpu::JP}},

    {0xC6, (Instr){(Instr_args){"ADD", 0xC6, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::IMM8}}, &Cpu::ADD}},
    {0xC7, (Instr){(Instr_args){"RST", 0xC7, (Operand){Addr_mode::IMPL_SHOW, NO_REG, 0}, (Operand){Addr_mode::IMPL}, Cond::ALWAYS, 1}, &Cpu::JP}},
    {0xC8, (Instr){(Instr_args){"RET", 0xC8, (Operand){Addr_mode::MEM16_REG, SP}, (Operand){Addr_mode::IMPL}, Cond::Z, 2}, &Cpu::JP}},
    {0xC9, (Instr){(Instr_args){"RET", 0xC9, (Operand){Addr_mode::MEM16_REG, SP}, (Operand){Addr_mode::IMPL}, Cond::ALWAYS, 2}, &Cpu::JP}},
    {0xCA, (Instr){(Instr_args){"JP", 0xCA, (Operand){Addr_mode::IMM16}, (Operand){Addr_mode::IMPL, NO_REG, 0}, Cond::Z}, &Cpu::JP}}, //OK
    //0xCB: Prefix
    {0xCC, (Instr){(Instr_args){"CALL", 0xCC, (Operand){Addr_mode::IMM16}, (Operand){Addr_mode::IMPL}, Cond::Z, 1}, &Cpu::JP}},
    {0xCD, (Instr){(Instr_args){"CALL", 0xCD, (Operand){Addr_mode::IMM16}, (Operand){Addr_mode::IMPL}, Cond::ALWAYS, 1}, &Cpu::JP}},
    {0xCE, (Instr){(Instr_args){"ADC", 0xCE, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::IMM8}, Cond::ALWAYS, 1}, &Cpu::ADD}},
    {0xCF, (Instr){(Instr_args){"RST", 0xCF, (Operand){Addr_mode::IMPL_SHOW, NO_REG, 8}, (Operand){Addr_mode::IMPL}, Cond::ALWAYS, 1}, &Cpu::JP}},
    {0xD0, (Instr){(Instr_args){"RET", 0xD0, (Operand){Addr_mode::MEM16_REG, SP}, (Operand){Addr_mode::IMPL}, Cond::NC, 2}, &Cpu::JP}},

    {0xD2, (Instr){(Instr_args){"JP", 0xD2, (Operand){Addr_mode::IMM16}, (Operand){Addr_mode::IMPL, NO_REG, 0}, Cond::NC}, &Cpu::JP}}, //OK
    {0xD3, (Instr){(Instr_args){"X_X", 0xD3}, &Cpu::X_X}},
    {0xD4, (Instr){(Instr_args){"CALL", 0xD4, (Operand){Addr_mode::IMM16}, (Operand){Addr_mode::IMPL}, Cond::NC, 1}, &Cpu::JP}},

    {0xD6, (Instr){(Instr_args){"SUB", 0xD6, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::IMM8}}, &Cpu::SUB}},
    {0xD7, (Instr){(Instr_args){"RST", 0xD7, (Operand){Addr_mode::IMPL_SHOW, NO_REG, 0x10}, (Operand){Addr_mode::IMPL}, Cond::ALWAYS, 1}, &Cpu::JP}},
    {0xD8, (Instr){(Instr_args){"RET", 0xD8, (Operand){Addr_mode::MEM16_REG, SP}, (Operand){Addr_mode::IMPL}, Cond::C, 2}, &Cpu::JP}},
    {0xD9, (Instr){(Instr_args){"RETI", 0xD9, (Operand){Addr_mode::MEM16_REG, SP}, (Operand){Addr_mode::IMPL}, Cond::ALWAYS, 3}, &Cpu::JP}},
    {0xDA, (Instr){(Instr_args){"JP", 0xDA, (Operand){Addr_mode::IMM16}, (Operand){Addr_mode::IMPL, NO_REG, 0}, Cond::C}, &Cpu::JP}}, //OK
    {0xDB, (Instr){(Instr_args){"X_X", 0xDB}, &Cpu::X_X}},
    {0xDC, (Instr){(Instr_args){"CALL", 0xDC, (Operand){Addr_mode::IMM16}, (Operand){Addr_mode::IMPL}, Cond::C, 1}, &Cpu::JP}},
    {0xDD, (Instr){(Instr_args){"X_X", 0xDD}, &Cpu::X_X}},
    {0xDE, (Instr){(Instr_args){"SBC", 0xDE, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::IMM8}, Cond::ALWAYS, 1}, &Cpu::SUB}},
    {0xDF, (Instr){(Instr_args){"RST", 0xDF, (Operand){Addr_mode::IMPL_SHOW, NO_REG, 0x18}, (Operand){Addr_mode::IMPL}, Cond::ALWAYS, 1}, &Cpu::JP}},
    {0xE0, (Instr){(Instr_args){"LD", 0xE0, (Operand){Addr_mode::HRAM_PLUS_IMM8}, (Operand){Addr_mode::REG, A}}, &Cpu::LD}},

    {0xE2, (Instr){(Instr_args){"LD", 0xE2, (Operand){Addr_mode::HRAM_PLUS_C}, (Operand){Addr_mode::REG, A}}, &Cpu::LD}},
    {0xE3, (Instr){(Instr_args){"X_X", 0xE3}, &Cpu::X_X}},
    {0xE4, (Instr){(Instr_args){"X_X", 0xE4}, &Cpu::X_X}},

    {0xE6, (Instr){(Instr_args){"AND", 0xE6, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::IMM8}}, &Cpu::AND}},
    {0xE7, (Instr){(Instr_args){"RST", 0xE7, (Operand){Addr_mode::IMPL_SHOW, NO_REG, 0x20}, (Operand){Addr_mode::IMPL}, Cond::ALWAYS, 1}, &Cpu::JP}},
    {0xE8, (Instr){(Instr_args){"ADD", 0xE8, (Operand){Addr_mode::REG16, SP}, (Operand){Addr_mode::IMMe8}}, &Cpu::ADD}},
    {0xE9, (Instr){(Instr_args){"JP", 0xE9, (Operand){Addr_mode::REG16, HL}}, &Cpu::JP}},
    {0xEA, (Instr){(Instr_args){"LD", 0xEA, (Operand){Addr_mode::MEM_IMM16}, (Operand){Addr_mode::REG, A}}, &Cpu::LD}},
    {0xEB, (Instr){(Instr_args){"X_X", 0xEB}, &Cpu::X_X}},
    {0xEC, (Instr){(Instr_args){"X_X", 0xEC}, &Cpu::X_X}},
    {0xED, (Instr){(Instr_args){"X_X", 0xED}, &Cpu::X_X}},
    {0xEE, (Instr){(Instr_args){"XOR", 0xEE, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::IMM8}}, &Cpu::XOR}},
    {0xEF, (Instr){(Instr_args){"RST", 0xEF, (Operand){Addr_mode::IMPL_SHOW, NO_REG, 0x28}, (Operand){Addr_mode::IMPL}, Cond::ALWAYS, 1}, &Cpu::JP}},

    {0xF4, (Instr){(Instr_args){"X_X", 0xF4}, &Cpu::X_X}},

    {0xF6, (Instr){(Instr_args){"OR", 0xF6, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::IMM8}}, &Cpu::OR}},
    {0xF7, (Instr){(Instr_args){"RST", 0xF7, (Operand){Addr_mode::IMPL_SHOW, NO_REG, 0x30}, (Operand){Addr_mode::IMPL}, Cond::ALWAYS, 1}, &Cpu::JP}},
    
    {0xF9, (Instr){(Instr_args){"LD", 0xF9, (Operand){Addr_mode::REG16, SP}, (Operand){Addr_mode::REG16, HL}}, &Cpu::LD}}, //OK

    {0xFC, (Instr){(Instr_args){"X_X", 0xFC}, &Cpu::X_X}},
    {0xFD, (Instr){(Instr_args){"X_X", 0xFD}, &Cpu::X_X}},
    {0xFE, (Instr){(Instr_args){"CP", 0xFE, (Operand){Addr_mode::REG, A}, (Operand){Addr_mode::IMM8}}, &Cpu::CP}},
    {0xFF, (Instr){(Instr_args){"RST", 0xFF, (Operand){Addr_mode::IMPL_SHOW, NO_REG, 0x38}, (Operand){Addr_mode::IMPL}, Cond::ALWAYS, 1}, &Cpu::JP}}
};
std::map<u8,Instr> Cpu::instr_map_prefix = {
    {0x00, (Instr){(Instr_args){"RLC B",0xCB00}, &Cpu::ROT}}
};
Cpu::Cpu(Memory& memory): mem(memory){
    this->reset();
}

void Cpu::reset(){
    this->state = RUNNING;
    this->IME_pending = 0;
    this->IME = false;
    this->halt_substate = HALT_SUBSTATE::NONE;
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
Int_Info Cpu::get_INTs(){
    Int_Info info;
    info.IE = this->mem[0xFFFF];
    info.IF = this->mem[0xFF0F];
    info.INT = info.IE & info.IF;
    return info;
}
void Cpu::set_IE(u16 value){
    this->mem[0xFFFF] = static_cast<u8>(value);
}
void Cpu::set_IF(u16 value){
    this->mem[0xFF0F] = static_cast<u8>(value);
}
bool Cpu::step(){
    if(this->state == RUNNING){
        Instr curr_instr;
        this->opcode = this->mem[this->regs[PC]];
        if (this->opcode != 0xCB)
            curr_instr = Cpu::instr_map[this->opcode];
        else{
            this->regs[PC]++;
            run_ticks(1);
            this->opcode = this->mem[this->regs[PC]];
            curr_instr = Cpu::instr_map_prefix[this->opcode];
        }
        this->regs[PC]++;
        run_ticks(1);
        (this->*(curr_instr.execute))(curr_instr.args);
        if (this->IME_pending > 0){
            this->IME_pending--;
            if (this->IME_pending == 0)
                this->IME = true;
        }
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
        case Addr_mode::IMPL: case Addr_mode::IMPL_SHOW:
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
        case Addr_mode::MEM16_REG:
            ticks_to_add = 2;
            op.value = this->mem[this->regs[op.reg]] | (this->mem[this->regs[op.reg]+1] << 8);
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
        case Addr_mode::MEM_REG: case Addr_mode::MEM16_REG:
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
    else{ //Assuming there is no broken instruction
        write_to_operand_16bit(op, value);
    }
}
bool Cpu::check_cond(Cond cond){
    switch(cond){
        case Cond::ALWAYS:
            return true;
            break;
        case Cond::C:
            return this->regs.get_flag(Flag::C);
            break;
        case Cond::NC:
            return !this->regs.get_flag(Flag::C);
            break;
        case Cond::Z:
            return this->regs.get_flag(Flag::Z);
            break;
        case Cond::NZ:
            return !this->regs.get_flag(Flag::Z);
            break;
    }
    std::cout<<"Erm... what the sigma? (check_cond)"<<std::endl;
    return false;
}
std::string Cpu::operand_toString(Operand op){
    std::string str = "";
    switch(op.addr_mode){
        case Addr_mode::IMPL: case Addr_mode::MEM16_REG:
            break;
        case Addr_mode::IMPL_SHOW:
            str+="($"+numToHexString(op.value, op.value > 0xFF? 4: 2)+")";
            break;
        case Addr_mode::IMM8: 
            this->fetch_operand(op, false);
            str += "$"+numToHexString(op.value, 2);
            break;
        case Addr_mode::IMMe8:
            this->fetch_operand(op, false);
            str += "$"+numToHexString(op.value, 2, true);
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
            str += reg_names[op.reg] + " + $" + numToHexString(op.value, 2, true);
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
    std::string dest_op_str =  this->operand_toString(args.dest);
    str += dest_op_str;
    std::string src_op_str = this->operand_toString(args.src);
    str += (dest_op_str.empty() || src_op_str.empty()?"":", ")+src_op_str;
    this->regs[PC]--;
    return str;
}
std::string Cpu::toString(){
    std::string str = "Cpu state: "+cpu_state_names[this->state];
    str += "\nRegisters:\n";
    for (int i=0; i<(int)(sizeof(reg_arr)/sizeof(reg_arr[0])-2); i++){
        str += reg_names[reg_arr[i]] + ": " + numToHexString(this->regs[reg_arr[i]], i<8 ? 2 : 4) + " ";
    }
    str+="\nFlags: ";
    for (int i = 0; i < size_flag_arr; i++){
        str += flag_names[flag_arr[i]] + ":" + std::to_string((u16)this->regs.get_flag(flag_arr[i])) + " ";
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
void Cpu::X_X(Instr_args args){
    std::cout<<"Parsed invalid opcode ("<<args.opcode<<") at "<<numToHexString(this->regs[PC], 4)<<std::endl;
    this->state = QUIT;
}
void Cpu::NOP(Instr_args args){
    return;
}
void Cpu::HALT(Instr_args args){
    this->state = HALTED;
    if(this->IME)
        this->halt_substate = HALT_SUBSTATE::IME_SET;
    else{
        if(this->get_INTs().INT == 0)
            this->halt_substate = HALT_SUBSTATE::NONE_PENDING;
        else
            this->halt_substate = HALT_SUBSTATE::SOME_PENDING;
    }
}
void Cpu::STOP(Instr_args args){
    this->NOP(args); //For now I'm leaving it this way, I didn't know STOP was so weird
}
void Cpu::LD(Instr_args args){
    this->fetch_operand(args.src);
    this->write_to_operand(args.dest, args.src.value, args.src.addr_mode);
}
void Cpu::PUSH_POP(Instr_args args){
    /*Variants
    0: PUSH
    1: POP
    */
    this->LD(args);
    this->regs[SP] += args.variant == 0 ? -2 : 2;
}

void Cpu::ADD(Instr_args args){
    /* Variants:
        0: ADD
        1: ADC
        2: INC
    */
    this->fetch_operand(args.src);
    this->fetch_operand(args.dest);
    u16 temp_result = args.dest.value + (args.src.addr_mode == Addr_mode::IMMe8 ? static_cast<int8_t>(args.src.value) : args.src.value);
    if (args.variant == 1) //If ADC
        temp_result += (u16)this->regs.get_flag(Flag::C);
    if(args.variant < 2 || args.dest.addr_mode != Addr_mode::REG16 ){ //INC r16 doesn't change the flags
        this->regs.set_flag(Flag::N, false);
        if(args.src.addr_mode != Addr_mode::REG16){ //If not adding 16 bit values
            if(args.src.addr_mode != Addr_mode::IMMe8)
                this->regs.set_flag(Flag::Z, (temp_result & 0xFF) == 0);
            else //Adding an e8 always sets the Z flag to false
                this->regs.set_flag(Flag::Z, false);
            this->regs.set_flag(Flag::H, (args.dest.value & 0xF) + (args.src.value & 0xF) > 0xF);
            if(args.variant != 2) //INC doesn't change the C flag
                this->regs.set_flag(Flag::C, temp_result > 0xFF);
        }else{
            this->regs.set_flag(Flag::H, (args.dest.value & 0xFFF) + (args.src.value & 0xFFF) > 0xFFF);
            this->regs.set_flag(Flag::C, temp_result < MIN(args.dest.value, args.src.value));
        }
    }
    this->write_to_operand(args.dest, temp_result, args.dest.addr_mode);
}
void Cpu::SUB(Instr_args args){
    /*Variants:
        0: SUB
        1: SBC
        2: DEC
    */
    this->fetch_operand(args.src);
    this->fetch_operand(args.dest);
    u16 temp_result = args.dest.value - args.src.value;
    if (args.variant == 1)
        temp_result -= (u16)this->regs.get_flag(Flag::C);
    if(args.variant < 2 || args.dest.addr_mode != Addr_mode::REG16 ){
        this->regs.set_flag(Flag::N, true);
        if (args.src.addr_mode != Addr_mode::REG16){
            this->regs.set_flag(Flag::Z, (temp_result & 0xFF) == 0);
            this->regs.set_flag(Flag::H, (args.dest.value & 0xF) < (args.src.value & 0xF));
        }else{
            this->regs.set_flag(Flag::H, (args.dest.value & 0xFFF) < (args.src.value & 0xFFF));
        }
        if (args.variant != 2)
            this->regs.set_flag(Flag::C, temp_result > args.dest.value);
    }
    this->write_to_operand(args.dest, temp_result, args.dest.addr_mode);
}
void Cpu::AND(Instr_args args){
    this->fetch_operand(args.src);
    this->fetch_operand(args.dest);
    u16 temp_result = args.dest.value & args.src.value;
    this->regs.set_flag(Flag::Z, temp_result == 0);
    this->regs.set_flag(Flag::N, false);
    this->regs.set_flag(Flag::H, true);
    this->regs.set_flag(Flag::C, false);
    this->write_to_operand(args.dest, temp_result, args.dest.addr_mode);
}
void Cpu::OR(Instr_args args){
    this->fetch_operand(args.src);
    this->fetch_operand(args.dest);
    u16 temp_result = args.dest.value | args.src.value;
    this->regs.set_flag(Flag::Z, temp_result == 0);
    this->regs.set_flag(Flag::N, false);
    this->regs.set_flag(Flag::H, false);
    this->regs.set_flag(Flag::C, false);
    this->write_to_operand(args.dest, temp_result, args.dest.addr_mode);
}
void Cpu::XOR(Instr_args args){
    this->fetch_operand(args.src);
    this->fetch_operand(args.dest);
    u16 temp_result = args.dest.value ^ args.src.value;
    this->regs.set_flag(Flag::Z, temp_result == 0);
    this->regs.set_flag(Flag::N, false);
    this->regs.set_flag(Flag::H, false);
    this->regs.set_flag(Flag::C, false);
    this->write_to_operand(args.dest, temp_result, args.dest.addr_mode);
}
void Cpu::CP(Instr_args args){
    this->fetch_operand(args.src);
    this->fetch_operand(args.dest);
    u16 temp_result = args.dest.value - args.src.value;
    this->regs.set_flag(Flag::Z, temp_result == 0);
    this->regs.set_flag(Flag::N, true);
    this->regs.set_flag(Flag::H, (args.dest.value & 0xF) < (args.src.value & 0xF));
    this->regs.set_flag(Flag::C, temp_result > args.dest.value);
}
void Cpu::CPL(Instr_args args){
    this->regs[A] = (~this->regs[A]) & 0xFF;
    this->regs.set_flag(Flag::N, true);
    this->regs.set_flag(Flag::H, true);
}
void Cpu::ROT(Instr_args args){
    /*Variants:
    0: RLA
    1: RL
    2: RLCA
    3: RLC
    4: RRA
    5: RR
    6: RRCA
    7: RRC
    */
    this->fetch_operand(args.dest);
    this->regs.set_flag(Flag::H, false);
    this->regs.set_flag(Flag::N, false);
    bool carry = this->regs.get_flag(Flag::C);
    u16 temp_result = 0;

    switch(args.variant){
        case 0: case 1:
            temp_result = (args.dest.value << 1) | carry;
            this->regs.set_flag(Flag::C, args.dest.value & 0x100);
            temp_result &= 0xFF;
            this->regs.set_flag(Flag::Z, temp_result == 0 && args.variant == 1);
            break;
        case 2: case 3:
            temp_result = (args.dest.value << 1) | (args.dest.value >> 7);
            this->regs.set_flag(Flag::C, args.dest.value & 0x80);
            temp_result &= 0xFF;
            this->regs.set_flag(Flag::Z, temp_result == 0 && args.variant == 3);
            break;
        case 4: case 5:
            temp_result |= this->regs.get_flag(Flag::C) << 8;
            this->regs.set_flag(Flag::C, temp_result & 1);
            temp_result >>= 1;
            this->regs.set_flag(Flag::Z, temp_result == 0 && args.variant == 5);
            break;
        case 6: case 7:
            temp_result = (args.dest.value >> 1) | (args.dest.value << 7);
            this->regs.set_flag(Flag::C, args.dest.value & 1);
            temp_result &= 0xFF;
            this->regs.set_flag(Flag::Z, temp_result == 0 && args.variant == 7);
            break;
        default:
            std::cout<<"Erm... what the sigma? (ROT)"<<std::endl;
            break;
    }
    this->write_to_operand(args.dest, temp_result, args.dest.addr_mode);
}
void Cpu::DAA(Instr_args args){
    /*
    This is probably the hardest instruction.
    Basically, we want to get the BCD representation of the A register.
    Usually, if A is, for example, 45, the value would be represented as 0x2D (0010 1101).
    After the BCD conversion, the value would change to 69, which is 0x45. That means that every nibble represents one digit of the number, which is very useful for score counters etc.
    
    DAA is supposed to be called after a valid BCD number has something added/subtracted to it, which may make it non-BCD (p.e: 0x45+4 = 0x49, OK, bu 0x45+5 = 0x4A, Not OK).
    The steps for DAA conversion are the following:
    1) If the operation was an addition
        a) If the second nibble is >9 (p.e: 0x45+5 = 0x4A instead of 0x50) or there has been a Half Carry (p.e: 0x49+7 = 0x50 instead of 0x56), add 6 to A
        b) If the first nibble is >9 (p.e: 0x99+7 = 0xA0 instead of 0x100*) or there has been a Full Carry (p.e: 0x99+0x67 = 0x100* instead of 0x166*), add 0x60 to A
    2) If the operation was a subtraction
        a) If there was a Half Carry (p.e: 0x10-1 = 0xF instead of 9), subtract 6 to A
        b) If there was a Full Carry (p.e: 0x160**-0x70 = 0xF0 instead of 0x90*), subtract 0x60 to A
    Finally, the C flag is updated accordingly
    *: Keep in mind that A is only 1 byte long, so you would only see the 2 rightmost bytes of these operations (though you could connect multiple 1 byte variables to make a bigger score counter, thanks to the changes that DAA makes to the C flag)
    **: Following the previous concept, keep in mind that DAA will assume that the minuend is always bigger than the sustraend, so it will represent 0x60-0x70 as 0x?60-0x70 = 0x90 and not as 0x10, since negative numbers are another story.
    */
    u8 offset = 0;
    bool new_carry = false;
    if (this->regs.get_flag(Flag::H) || (!this->regs.get_flag(Flag::N) && (this->regs[A] & 0xF) > 9))
        offset = 6;
    if (this->regs.get_flag(Flag::C) || (!this->regs.get_flag(Flag::N) && (this->regs[A] & 0xF) > 0x99))
        offset |= 0x60;
        new_carry = true;
    this->regs[A] += this->regs.get_flag(Flag::N) ? -offset : offset;
    this->regs.set_flag(Flag::C, new_carry);
    this->regs.set_flag(Flag::H, false);
    this->regs.set_flag(Flag::Z, this->regs[A] == 0);
}
void Cpu::JP(Instr_args args){
    /*Variants:
        0: JP/JR
        1: CALL/RST
        2: RET
        3: RETI
    */
    if(args.dest.addr_mode == Addr_mode::IMM16 || args.dest.addr_mode == Addr_mode::IMMe8){
        fetch_operand(args.dest);
        if(args.dest.addr_mode == Addr_mode::IMMe8)
            args.dest.value = this->regs[PC] + static_cast<int8_t>(args.dest.value);
    }
    if(check_cond(args.cond)){
        if (args.dest.addr_mode != Addr_mode::IMM16 && args.dest.addr_mode != Addr_mode::IMMe8){
            fetch_operand(args.dest);
        }
        if(args.variant == 1){
            this->regs[SP] -= 2;
            Operand storeOp = (Operand){Addr_mode::MEM16_REG, SP};
            write_to_operand(storeOp, this->regs[PC], Addr_mode::REG16);
        }
        else if (args.variant >= 2){
            this->regs[SP] += 2;
            if(args.variant == 3)
                this->IME = true;
        }
        this->regs[PC] = args.dest.value;
    }
}