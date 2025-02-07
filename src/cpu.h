#pragma once
#include <utils.h>

#include <map>
#include <algorithm>
#include <string>
typedef enum {RUNNING, PAUSED, STOPPED, HALTED, QUIT} Cpu_State;

class Memory;

enum class Addr_mode{IMPL, IMPL_SHOW, IMM8, IMMe8, IMM16, REG, REG16, REG16_PLUS_IMMe8, MEM_REG, MEM_REG_INC, MEM_REG_DEC, MEM16_REG, MEM_IMM16, HRAM_PLUS_IMM8, HRAM_PLUS_C}; //All types of addressing modes for each instruction

enum class Cond{ALWAYS, Z, NZ, C, NC};
enum class Flag{Z, N, H, C};
extern std::map<Cpu_State, std::string> cpu_state_names; //Map of cpu states to their string representation
extern std::map<Flag, u16> flag_despl; //Indicates the bit of each flag in F
extern std::map<Flag,std::string> flag_names; // Map of flags to their string representation
extern Flag flag_arr[]; //Array of all flags
extern int size_flag_arr;
//typedef enum {REG_A, REG_F, REG_B, REG_C, REG_D, REG_E, REG_H, REG_L, REG_SP, REG_PC, REG_AF, BC, DE, HL} Reg;
typedef enum {A, F, B, C, D, E, H, L, SP, PC, AF, BC, DE, HL, NO_REG} Reg;
extern Reg reg_arr[]; //Array of all registers
extern int size_reg_arr;
extern Reg composite_regs[]; //Array of all composite registers
extern int size_composite_regs;
extern Reg byte_regs[]; //Array of all simple registers
extern int size_byte_regs;
extern std::map<Reg, std::pair<Reg, Reg>> reg_pairs; //Maps a composite register to its two simple registers
extern std::map<Reg, std::string> reg_names; //Map of registers to their string representation
extern std::map<std::string, Reg> reg_map; //Map of string representation of registers to their enum
extern std::map<Cond, std::string> cond_names; //Map of conditions to their string representation

typedef struct{
    Addr_mode addr_mode = Addr_mode::IMPL;
    Reg reg = NO_REG;
    u16 value = 0;
} Operand; //Struct that represents an operand in an instruction

class Reg_dict {
    /*This class allows access to registers as if they were a dictionary.
     *It also handels all synchronization between composite registers and their simple registers.
     *It also allows easy access and modification of flags in the F register.
     */
    private:
        std::map<Reg, u16> regs;

        class Proxy { //This allows to customize the functionality of the [], = and various operators for the Reg_dict class
            private:
                Reg_dict& parent;
                Reg reg;
            public:
                Proxy(Reg_dict& parent, Reg reg) : parent(parent), reg(reg) {}

                operator u16() const { // Reading value (either simple or composite)
                    if (std::find(composite_regs, composite_regs + size_composite_regs, reg) == composite_regs + size_composite_regs) { //simple
                        return parent.regs.at(reg);
                    } else { //composite
                        u16 composite_value = (static_cast<u16>(parent.regs.at(reg_pairs.at(reg).first)) << 8) | parent.regs.at(reg_pairs.at(reg).second);
                        return composite_value;
                    }
                }
                void set(u16 value) { //Baseline for assigning a value to a register
                    if (std::find(composite_regs, composite_regs + size_composite_regs, reg) == composite_regs + size_composite_regs) { //simple
                        if (std::find(byte_regs, byte_regs + size_byte_regs, reg) != byte_regs + size_byte_regs) { //8 bit simple (excluding PC and SP)
                            value &= 0xFF;
                        }
                        parent.regs[reg] = value;
                    } else { //composite
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

        Proxy operator[](Reg reg) { //Caller for the Proxy class
            return Proxy(*this, reg);
        }
        bool get_flag(Flag flag) {
            return (regs[F] >> flag_despl[flag]) & 1;
        }
        void set_flag(Flag flag, bool value) {
            if (value) {
                regs[F] |= 1 << flag_despl[flag];
            } else {
                regs[F] &= ~(1 << flag_despl[flag]);
            }
        }

};

class Cpu;
typedef struct{
    std::string name; //String representation of the instruction
    u16 opcode; //Opcode of the instruction (8 bit if not prefixed, CBxx if prefixed)
    Operand dest = {Addr_mode::IMPL, NO_REG, 0};
    Operand src = {Addr_mode::IMPL, NO_REG, 0};
    Cond cond = Cond::ALWAYS;
    int variant = 0; //Some functions attend to different, yet similar instructions. variant is used to differentiate between them
} Instr_args; //Struct that represents the arguments of an instruction. Only the name and opcode are mandatory, the rest are filled as needed
typedef struct{
    Instr_args args; //All arguments of the instruction
    void (Cpu::*execute)(Instr_args args); //Pointer to the function that executes the instruction
} Instr;
enum class HALT_SUBSTATE{NONE, IME_SET, NONE_PENDING, SOME_PENDING}; //Subtypes of the HALT state, as defined in the Z80 guide
typedef struct{
    u8 IE;
    u8 IF;
    u8 INT;
} Int_Info; //Struct that represents the state of the interrupt system and allows easy access to the IE, IF and INT registers
class Cpu{
    private:
        Memory& mem;
        void fetch_operand(Operand& op, bool affect=true);
        void write_to_operand_8bit(Operand &op, u16 value);
        void write_to_operand_16bit(Operand &op, u16 value);
        void write_to_operand(Operand &op, u16 value, Addr_mode src_addr = Addr_mode::IMPL);
        bool check_cond(Cond cond);
        std::string operand_toString(Operand op);
        std::string instr_toString(Instr instr);
        static std::map<u8, Instr> instr_map;
        static std::map<u8, Instr> instr_map_prefix;
        u16 int_addrs[5] = {0x40, 0x48, 0x50, 0x58, 0x60};
        #pragma region All_instructions
            void noImpl(Instr_args args);
            void X_X(Instr_args args);
            void NOP(Instr_args args);
            void STOP(Instr_args args);
            void HALT(Instr_args args);
            void DI(Instr_args args);
            void EI(Instr_args args);
            void LD(Instr_args args);
            void PUSH_POP(Instr_args args);
            void ADD(Instr_args args);
            void SUB(Instr_args args);
            void AND(Instr_args args);
            void OR(Instr_args args);
            void XOR(Instr_args args);
            void CP(Instr_args args);
            void CPL(Instr_args args);
            void JP(Instr_args args);
            void ROT(Instr_args args);
            void SHIFT(Instr_args args);
            void SWAP(Instr_args args);
            void BIT(Instr_args args);
            void RES(Instr_args args);
            void SET(Instr_args args);
            void DAA(Instr_args args);
            void SCF(Instr_args args);
            void CCF(Instr_args args);
        #pragma endregion
    public:
        Cpu_State state = RUNNING;
        bool IME = false;
        u8 IME_pending;
        HALT_SUBSTATE halt_substate;
        u8 opcode;
        u16* src;
        u16* dest;
        Reg_dict regs;
        Cpu(Memory& mem);
        bool step();
        Int_Info get_INTs();
        void set_IE(u16 value);
        void set_IF(u16 value);
        void check_interrupts();
        

        std::string toString();
        
        void reset();
};

