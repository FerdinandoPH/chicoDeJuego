#pragma once
#include <utils.h>

#include <map>
#include <algorithm>
#include <string>
typedef enum {RUN, PAUSE, STOP, HALT, QUIT} Cpu_State;

class Memory;
//typedef enum {IMM, REG, MEM, REG_IMM} Addr_mode;
enum class Addr_mode{IMPL, IMM8, IMMe8, IMM16, REG, REG16, REG16_PLUS_IMMe8, MEM_REG, MEM_REG_INC, MEM_REG_DEC, MEM_IMM16, HRAM_PLUS_IMM8, HRAM_PLUS_C};
//typedef enum {ALWAYS, Z, NZ, C, NC} Cond;
enum class Cond{ALWAYS, Z, NZ, C, NC};
//typedef enum {REG_A, REG_F, REG_B, REG_C, REG_D, REG_E, REG_H, REG_L, REG_SP, REG_PC, REG_AF, BC, DE, HL} Reg;
typedef enum {A, F, B, C, D, E, H, L, SP, PC, AF, BC, DE, HL, NO_REG} Reg;
extern Reg reg_arr[];
extern int size_reg_arr;
extern Reg composite_regs[];
extern int size_composite_regs;
extern Reg byte_regs[];
extern int size_byte_regs;
extern std::map<Reg, std::pair<Reg, Reg>> reg_pairs;
extern std::map<Reg, std::string> reg_names;
extern std::map<Cond, std::string> cond_names;

typedef struct{
    Addr_mode addr_mode = Addr_mode::IMPL;
    Reg reg = NO_REG;
    u16 value = 0;
} Operand;

class Reg_dict {
    private:
        std::map<Reg, u16> regs;

        class Proxy {
            private:
                Reg_dict& parent;
                Reg reg;
            public:
                Proxy(Reg_dict& parent, Reg reg) : parent(parent), reg(reg) {}

                operator u16() const {
                    if (std::find(composite_regs, composite_regs + size_composite_regs, reg) == composite_regs + size_composite_regs) {
                        return parent.regs.at(reg);
                    } else {
                        u16 composite_value = (static_cast<u16>(parent.regs.at(reg_pairs.at(reg).first)) << 8) | parent.regs.at(reg_pairs.at(reg).second);
                        return composite_value;
                    }
                }
                void set(u16 value) {
                    if (std::find(composite_regs, composite_regs + size_composite_regs, reg) == composite_regs + size_composite_regs) {
                        if (std::find(byte_regs, byte_regs + size_byte_regs, reg) != byte_regs + size_byte_regs) {
                            value &= 0xFF;
                        }
                        parent.regs[reg] = value;
                    } else {
                        parent.regs[reg_pairs[reg].first] = value >> 8;
                        parent.regs[reg_pairs[reg].second] = value & 0xFF;
                    }
                }
                Proxy& operator=(u16 value) {
                    this->set(value);
                    return *this;
                }
                Proxy& operator++() {
                    this->set(this->operator u16() + 1);
                    return *this;
                }
                Proxy& operator++(int) {
                    this->set(this->operator u16() + 1);
                    return *this;
                }
                Proxy& operator--() {
                    this->set(this->operator u16() - 1);
                    return *this;
                }
                Proxy& operator--(int) {
                    this->set(this->operator u16() - 1);
                    return *this;
                }
                Proxy& operator+=(u16 value) {
                    this->set(this->operator u16() + value);
                    return *this;
                }
                Proxy& operator-=(u16 value) {
                    this->set(this->operator u16() - value);
                    return *this;
                }
                Proxy& operator*=(u16 value) {
                    this->set(this->operator u16() * value);
                    return *this;
                }
                Proxy& operator/=(u16 value) {
                    this->set(this->operator u16() / value);
                    return *this;
                }
                Proxy& operator%=(u16 value) {
                    this->set(this->operator u16() % value);
                    return *this;
                }
                Proxy& operator|=(u16 value) {
                    this->set(this->operator u16() | value);
                    return *this;
                }
                Proxy& operator&=(u16 value) {
                    this->set(this->operator u16() & value);
                    return *this;
                }
                Proxy& operator^=(u16 value) {
                    this->set(this->operator u16() ^ value);
                    return *this;
                }
                Proxy& operator<<=(u16 value) {
                    this->set(this->operator u16() << value);
                    return *this;
                }
                Proxy& operator>>=(u16 value) {
                    this->set(this->operator u16() >> value);
                    return *this;
                }



        };

    public:
        Reg_dict() {
            for (int i = 0; i < size_reg_arr; i++) {
                regs[reg_arr[i]] = 0;
            }
        }

        Proxy operator[](Reg reg) {
            return Proxy(*this, reg);
        }

};

class Cpu;
typedef struct{
    std::string name;
    u16 opcode;
    Operand dest = {Addr_mode::IMPL, NO_REG, 0};
    Operand src = {Addr_mode::IMPL, NO_REG, 0};
    Cond cond = Cond::ALWAYS;

} Instr_args;
typedef struct{
    Instr_args args;
    void (Cpu::*execute)(Instr_args args);
} Instr;
class Cpu{
    private:
        Memory& mem;
        void fetch_operand(Operand& op, bool affect=true);
        void write_to_operand_8bit(Operand &op, u16 value);
        void write_to_operand_16bit(Operand &op, u16 value);
        void write_to_operand(Operand &op, u16 value, Addr_mode src_addr = Addr_mode::IMPL);
        std::string operand_toString(Operand op);
        std::string instr_toString(Instr instr);
        static std::map<u8, Instr> instr_map;
        static std::map<u8, Instr> instr_map_prefix;
        #pragma region All_instructions
            void noImpl(Instr_args args);
            void NOP(Instr_args args);
            void LD(Instr_args args);
            void INC(Instr_args args);
            void DEC(Instr_args args);
            void RLC(Instr_args args);
            void ADD(Instr_args args);
            void SUB(Instr_args args);
            void JP(Instr_args args);
        #pragma endregion
    public:
        Cpu_State state = RUN;
        u8 opcode;
        u16* src;
        u16* dest;
        Reg_dict regs;
        Cpu(Memory& mem);
        bool step();


        std::string toString();
        
        void reset();
};

