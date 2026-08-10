#include "debug_api.h"
#include "emu.h"
#include "cpu.h"
#include "memory.h"
#include "host.h"
#include <algorithm>

Debug_api::Debug_api(Debugger& dbg, Cpu& cpu, Memory& mem, Host& host) : dbg(dbg), cpu(cpu), mem(mem), host(host) {}

// --- Machine state ---

u8 Debug_api::read_mem(u16 addr){
    return this->mem.readX(addr);
}
u16 Debug_api::read_reg(Reg reg){
    return this->cpu.regs[reg];
}
std::vector<u16> Debug_api::last_pc_values(){
    return this->dbg.last_pc_values;
}

// --- Breakpoints ---

std::string Debug_api::breakpoints_toString(){
    return this->dbg.breakpoints_toString();
}
void Debug_api::add_pos_breakpoint(u16 pos){
    this->dbg.add_pos_breakpoint(pos);
}
void Debug_api::add_opcode_breakpoint(u8 opcode){
    this->dbg.add_opcode_breakpoint(opcode);
}
void Debug_api::add_mem_breakpoint(u16 addr, Dbg_cond cond, u8 value, u8 value2, bool current){
    this->dbg.add_mem_breakpoint(addr, cond, value, value2, current);
}
void Debug_api::add_reg_breakpoint(Reg reg, Dbg_cond cond, u16 value, u16 value2, bool current){
    this->dbg.add_reg_breakpoint(reg, cond, value, value2, current);
}
bool Debug_api::del_breakpoint(Breakpoint_type type, size_t index){
    return this->dbg.del_breakpoint(type, index);
}
void Debug_api::clear_breakpoints(){
    this->dbg.clear_breakpoints();
}

// --- Views ---

bool Debug_api::is_debug_window_active(DebugWindowType type){
    return this->host.is_debug_window_active(type);
}
void Debug_api::toggle_debug_window(DebugWindowType type){
    // The window is not opened here: creating and destroying it belongs to the
    // thread that owns the backend, so all we do is leave the request behind.
    this->host.debug_toggle_requested[(int)type] = true;
}
void Debug_api::clear_main_screen(){
    this->host.clear_main_screen();
}
bool Debug_api::dump_memory(){
    if (!this->mem.dump())
        return false;
    this->host.open_with_default_app("mem.hexd"); // opens it with whatever the system uses for .hexd
    return true;
}

// --- Run control ---

void Debug_api::step(){
    this->dbg.dbg_level.store(FULL_DBG, std::memory_order_relaxed);
}
void Debug_api::resume(){
    this->dbg.dbg_level.store(OFF_DBG, std::memory_order_relaxed);
}
void Debug_api::quit(){
    this->cpu.set_state(QUIT);
}
void Debug_api::reset(){
    emu_reset();
}

// --- Text parsing ---

bool parse_hex(const std::string& text, u16& out){
    try{
        out = (u16)std::stoi(text, nullptr, 16);
        return true;
    }
    catch(...){
        return false;
    }
}
bool parse_cond(const std::string& text, Dbg_cond& out){
    auto it = dbg_cond_map.find(text);
    if (it == dbg_cond_map.end())
        return false;
    out = it->second;
    return true;
}
bool parse_reg(const std::string& text, Reg& out){
    std::string upper = text;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    auto it = reg_map.find(upper);
    if (it == reg_map.end())
        return false;
    out = it->second;
    return true;
}
