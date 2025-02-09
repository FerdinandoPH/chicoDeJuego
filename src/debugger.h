#pragma once
#include <utils.h>
#include <memory.h>
#include <cpu.h>
#include <timer.h>
#include <ppu.h>
#include <vector>
typedef enum {NO_DBG, OFF_DBG, PRINT_DBG, FULL_DBG} Debug_mode;
enum class Dbg_cond{EQ, NEQ, GT, LT, GTE, LTE};
enum class Breakpoint_type{POS, OPCODE, MEM, REG};
typedef struct{
    u16 addr;
    Dbg_cond cond;
    u8 value;
    bool current = false;
} Mem_breakpoint;
typedef struct{
    Reg reg;
    Dbg_cond cond;
    u16 value;
    bool current = false;
} Reg_breakpoint;
extern std::map<std::string, Dbg_cond> dbg_cond_map;
extern std::map<Dbg_cond, std::string> dbg_cond_names;
class Debugger{
    private:
        int& ticks;
        Memory& mem;
        Cpu& cpu;
        Timer& timer;
        Ppu& ppu;
        std::vector<u16> pos_breakpoints;
        std::vector<u16> opcode_breakpoints;
        std::vector<Mem_breakpoint> mem_breakpoints;
        std::vector<Reg_breakpoint> reg_breakpoints;
        static bool check_dbg_cond(Dbg_cond cond, u16 val1, u16 val2);
    public:
        Debug_mode dbg_level;
        Debugger(int& ticks, Memory& mem, Cpu& cpu, Timer& timer, Ppu& ppu);
        void debug_print();
        bool check_breakpoints();

        void add_breakpoint_menu();
        bool add_pos_breakpoint(std::string pos);
        bool add_opcode_breakpoint(std::string opcode);
        bool add_mem_breakpoint(std::string addr, std::string cond, std::string value);
        bool add_reg_breakpoint(std::string reg, std::string cond, std::string value);

        void del_breakpoint_menu();
        bool del_breakpoint(Breakpoint_type br_type, size_t index);
        void clear_breakpoints();
        std::string breakpoints_toString();
};


