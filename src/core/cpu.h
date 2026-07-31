#pragma once
#include "utils.h"
#include "hw_reg_def.h"

#include <unordered_map>
#include <algorithm>
#include <string>
#include <mutex>
#include <atomic>
typedef enum {RUNNING, PAUSED, STOPPED, HALTED, VDMA_HALTED, QUIT} Cpu_State;

class Memory;

enum class Speed_mode{NORMAL, DOUBLE};
enum class Addr_mode{IMPL, IMPL_SHOW, IMM8, IMMe8, IMM16, REG, REG16, REG16_PLUS_IMMe8, MEM_REG, MEM_REG_INC, MEM_REG_DEC, MEM16_REG, MEM_IMM16, HRAM_PLUS_IMM8, HRAM_PLUS_C}; //All types of addressing modes for each instruction

enum class Cond{ALWAYS, Z, NZ, C, NC};
enum class Flag{Z, N, H, C};
extern const std::unordered_map<Cpu_State, std::string> cpu_state_names; //Map of cpu states to their string representation
extern const std::unordered_map<Flag, u16> flag_despl; //Indicates the bit of each flag in F
extern const std::unordered_map<Flag,std::string> flag_names; // Map of flags to their string representation
extern const Flag flag_arr[]; //Array of all flags
extern const int size_flag_arr;
//typedef enum {REG_A, REG_F, REG_B, REG_C, REG_D, REG_E, REG_H, REG_L, REG_SP, REG_PC, REG_AF, BC, DE, HL} Reg;
typedef enum {A=0, F=1, B=2, C=3, D=4, E=5, H=6, L=7, SP=8, PC=9, AF=10, BC=11, DE=12, HL=13, NO_REG=14} Reg;
extern const Reg reg_arr[]; //Array of all registers
extern const int size_reg_arr;
extern const Reg composite_regs[]; //Array of all composite registers
extern const int size_composite_regs;
extern const Reg byte_regs[]; //Array of all simple registers
extern const int size_byte_regs;
extern const std::unordered_map<Reg, std::pair<Reg, Reg>> reg_pairs; //Maps a composite register to its two simple registers
extern const std::unordered_map<Reg, std::string> reg_names; //Map of registers to their string representation
extern const std::unordered_map<std::string, Reg> reg_map; //Map of string representation of registers to their enum
extern const std::unordered_map<Cond, std::string> cond_names; //Map of conditions to their string representation

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
        u8 r8[8];
        u16 sp = 0;
        u16 pc = 0;

        class Proxy { //This allows to customize the functionality of the [], = and various operators for the Reg_dict class
            private:
                Reg_dict& parent;
                Reg reg;
            public:
                Proxy(Reg_dict& parent, Reg reg) : parent(parent), reg(reg) {}

                operator u16() const {
                    switch (reg) {
                        case A: case F: case B: case C:
                        case D: case E: case H: case L:
                            return parent.r8[reg];
                        case SP: return parent.sp;
                        case PC: return parent.pc;
                        case AF: return (u16(parent.r8[A]) << 8) | parent.r8[F];
                        case BC: return (u16(parent.r8[B]) << 8) | parent.r8[C];
                        case DE: return (u16(parent.r8[D]) << 8) | parent.r8[E];
                        case HL: return (u16(parent.r8[H]) << 8) | parent.r8[L];
                        default: return 0;
                    }
                }
                void set(u16 value) { //Baseline for assigning a value to a register
                    switch (reg) {
                        case F: parent.r8[F] = value & 0xF0; break;
                        case A: case B: case C: case D:
                        case E: case H: case L:
                            parent.r8[reg] = value & 0xFF; break;
                        case SP: parent.sp = value; break;
                        case PC: parent.pc = value; break;
                        case AF: parent.r8[A] = value >> 8; parent.r8[F] = value & 0xF0; break;
                        case BC: parent.r8[B] = value >> 8; parent.r8[C] = value & 0xFF; break;
                        case DE: parent.r8[D] = value >> 8; parent.r8[E] = value & 0xFF; break;
                        case HL: parent.r8[H] = value >> 8; parent.r8[L] = value & 0xFF; break;
                        default: break;
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
            for (int i = 0; i < 8; i++) {
                r8[i] = 0;
                pc = 0;
                sp = 0;
            }
        }

        Proxy operator[](Reg reg) { //Caller for the Proxy class
            return Proxy(*this, reg);
        }
        bool get_flag(Flag flag) {
            return (r8[F] >> flag_despl.at(flag)) & 1;
        }
        void set_flag(Flag flag, bool value) {
            if (value) {
                r8[F] |= 1 << flag_despl.at(flag);
            } else {
                r8[F] &= ~(1 << flag_despl.at(flag));
            }
        }
        static inline bool is_composite_reg(Reg reg) {
            switch(reg){
                case AF: case BC: case DE: case HL:
                    return true;
                default:
                    return false;
            }
        }
        static inline bool is_byte_reg(Reg reg){
            switch(reg){
                case A: case F: case B: case C: case D: case E: case H: case L:
                    return true;
                default:
                    return false;
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
typedef struct{
    u16 pc;
    u16 sp;
    u8 a, f, b, c, d, e, h, l;
    u8 IF,IE;
    bool IME;
}Cpu_trace;
// Savestate
typedef struct{
    u8 a, f, b, c, d, e, h, l;
    u16 sp, pc;
}Reg_ss;
typedef struct{
    Reg_ss reg;
    bool IME;
    u8 IME_pending;
    Cpu_State state;
    HALT_SUBSTATE halt_substate;
}Cpu_ss;

class Timer;
class Apu;
class Cpu{
    private:
        std::atomic<Cpu_State> state{RUNNING};
        Memory& mem;
        Timer* timer;
        Apu* apu;
        GB_model& gb_model;
        int& ticks_per_frame;
        Speed_mode speed_mode = Speed_mode::NORMAL;
        void fetch_operand(Operand& op, bool affect=true);
        void write_to_operand_8bit(Operand &op, u16 value, Addr_mode src_addr);
        void write_to_operand_16bit(Operand &op, u16 value, Addr_mode src_addr);
        void write_to_operand(Operand &op, u16 value, Addr_mode src_addr = Addr_mode::IMPL);
        bool check_cond(Cond cond);
        std::string operand_toString(Operand op);
        std::string instr_toString(Instr instr);
        //static std::map<u8, Instr> instr_map;
        static const Instr instr_table[0x100];
        //static std::map<u8, Instr> instr_map_prefix;
        static const Instr instr_table_prefix[0x100];
        static const u16 int_addrs[5];
        #ifdef LOGGER
        FILE* log;
        u16 prev_pc = 0xFFFF;
        #endif
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
            void BIT_INSTR(Instr_args args);
            void RES(Instr_args args);
            void SET(Instr_args args);
            void DAA(Instr_args args);
            void SCF(Instr_args args);
            void CCF(Instr_args args);
        #pragma endregion
    public:
        
        bool IME = false;
        u8 IME_pending;
        HALT_SUBSTATE halt_substate;
        u8 opcode;
        Reg_dict regs;
        Cpu(Memory& mem, GB_model& gb_model, int& ticks_per_frame);
        #ifdef LOGGER
        ~Cpu();
        #endif
        void set_timer(Timer* timer){
            this->timer = timer;
        };
        void set_apu(Apu* apu){
            this->apu = apu;
        };
        bool step();
        Cpu_State get_state();
        bool set_state(Cpu_State new_state);
        Int_Info get_INTs();
        void set_IE(u16 value);
        void set_IF(u16 value);
        bool check_interrupts();
        void adjust_flag_from_checksum();

        Speed_mode get_speed_mode(){
            return this->speed_mode;
        };

        std::string toString();

        void reset();
        Cpu_trace get_trace();
        Cpu_ss save_state();
        void load_state(const Cpu_ss& state);
};
