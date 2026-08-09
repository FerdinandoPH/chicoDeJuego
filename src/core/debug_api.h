#pragma once
#include "utils.h"
#include "debugger.h"
#include "video.h"
#include <string>
#include <vector>

class Cpu;
class Memory;
class Host;

// Everything the debug menu is allowed to do, as verbs and queries that return
// data. Nothing here prints anything or reads anything: that is the menu's job,
// and the menu belongs to the backend, because "ask the user for a hex address"
// means something very different on a terminal and on a console with a d-pad.
//
// Adding a command to the emulator means adding a method here; how it gets
// invoked is then each backend's problem.
class Debug_api{
    private:
        Debugger& dbg;
        Cpu& cpu;
        Memory& mem;
        Host& host;
    public:
        Debug_api(Debugger& dbg, Cpu& cpu, Memory& mem, Host& host);

        // --- Machine state ---
        // Reading these is only safe while the emulation thread is stopped,
        // which is exactly when the menu runs.
        u8  read_mem(u16 addr);
        u16 read_reg(Reg reg);
        std::vector<u16> last_pc_values();

        // --- Breakpoints ---
        std::string breakpoints_toString();
        void add_pos_breakpoint(u16 pos);
        void add_opcode_breakpoint(u8 opcode);
        void add_mem_breakpoint(u16 addr, Dbg_cond cond, u8 value, u8 value2 = 0, bool current = false);
        void add_reg_breakpoint(Reg reg, Dbg_cond cond, u16 value, u16 value2 = 0, bool current = false);
        bool del_breakpoint(Breakpoint_type type, size_t index);
        void clear_breakpoints();

        // --- Views ---
        bool is_debug_window_active(DebugWindowType type);
        void toggle_debug_window(DebugWindowType type);
        void clear_main_screen();
        bool dump_memory();  // writes mem.hexd and hands it to the system viewer

        // --- Run control ---
        // step and resume both end the menu; the difference is whether the next
        // instruction stops again.
        void step();
        void resume();
        void quit();
};

// Turning text into the arguments the API takes. They live here and not in the
// backend because every text-driven UI needs the same thing, and they are the
// only place that still assumes anything was typed at all.
bool parse_hex(const std::string& text, u16& out);
bool parse_cond(const std::string& text, Dbg_cond& out);
bool parse_reg(const std::string& text, Reg& out);
