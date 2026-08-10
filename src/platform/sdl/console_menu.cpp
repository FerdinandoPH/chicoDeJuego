#include "menu.h"
#include "debug_api.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

// The debug menu for the desktop backend: a terminal. This used to live in
// src/core (emu.cpp and debugger.cpp) and is the same menu, with the same keys;
// what changed is that it no longer touches the emulator directly, only
// Debug_api. That is what lets a backend with no keyboard replace this file with
// an on-screen menu without the core noticing.
//
// It runs on the emulation thread, which is stopped for as long as we are here.
// The window keeps being redrawn by the main thread meanwhile, as it always did.

static std::string read_line(){
    std::string line;
    std::getline(std::cin, line);
    return line;
}

// Same as the parse_* helpers in the core, for the plain decimal indices the
// delete menu asks for.
static bool parse_index(const std::string& text, size_t& out){
    try{
        int value = std::stoi(text);
        if (value < 0) return false;
        out = (size_t)value;
        return true;
    }
    catch(...){
        return false;
    }
}

// Reads the "condition + value [+ second value]" tail shared by the memory and
// register breakpoints. A value of X means "whatever it is right now", which is
// what makes a != breakpoint fire on every change.
static bool read_cond_and_values(Debug_api& api, Dbg_cond& cond, u16& value, u16& value2,
                                 bool& current, bool reg_breakpoint, u16 addr, Reg reg){
    printf("Enter condition (==, !=, >, <, >=, <=, ><): ");
    if (!parse_cond(read_line(), cond))
        return false;

    printf("Enter value: ");
    std::string value_str = read_line();
    if (value_str == "X" || value_str == "x"){
        value   = reg_breakpoint ? api.read_reg(reg) : api.read_mem(addr);
        current = true;
    }
    else if (!parse_hex(value_str, value)){
        return false;
    }

    value2 = 0;
    if (cond == Dbg_cond::BET){
        printf("Enter second value: ");
        if (!parse_hex(read_line(), value2))
            return false;
    }
    return true;
}

static void add_breakpoint_menu(Debug_api& api){
    bool ok = false;
    while (!ok){
        printf("Enter breakpoint type:\n");
        printf("p: Position\n");
        printf("o: Opcode\n");
        printf("m: Memory\n");
        printf("r: Register\n");
        printf("x: Cancel\n");
        std::string type = read_line();
        u16 pos, opcode, addr, value, value2;
        Reg reg;
        Dbg_cond cond;
        bool current;
        switch(type[0]){
            case 'p':
                printf("Enter position: ");
                ok = parse_hex(read_line(), pos);
                if (ok){
                    api.add_pos_breakpoint(pos);
                    std::cout<<"Added pos breakpoint at "<<numToHexString(pos, 4)<<std::endl;
                }
                break;
            case 'o':
                printf("Enter opcode: ");
                ok = parse_hex(read_line(), opcode);
                if (ok){
                    api.add_opcode_breakpoint((u8)opcode);
                    std::cout<<"Added opcode breakpoint at "<<numToHexString((u8)opcode, 2)<<std::endl;
                }
                break;
            case 'm':
                printf("Enter address: ");
                current = false;
                ok = parse_hex(read_line(), addr) &&
                     read_cond_and_values(api, cond, value, value2, current, false, addr, NO_REG);
                if (ok){
                    api.add_mem_breakpoint(addr, cond, (u8)value, (u8)value2, current);
                    std::cout<<"Added mem breakpoint at "<<numToHexString(addr, 4)<<" with condition "<<dbg_cond_names.at(cond)<<" and value "<<numToHexString((u8)value, 2)<<(cond == Dbg_cond::BET?("<->"+numToHexString((u8)value2, 2)):"")<< std::endl;
                }
                break;
            case 'r':
                printf("Enter register: ");
                current = false;
                ok = parse_reg(read_line(), reg) &&
                     read_cond_and_values(api, cond, value, value2, current, true, 0, reg);
                if (ok){
                    api.add_reg_breakpoint(reg, cond, value, value2, current);
                    std::cout<<"Added reg breakpoint at "<<reg_names.at(reg)<<" with condition "<<dbg_cond_names.at(cond)<<" and value "<<numToHexString(value)<<(cond == Dbg_cond::BET?("<->"+numToHexString(value2)):"")<< std::endl;
                }
                break;
            case 'x':
                ok = true;
                break;
            default:
                printf("Invalid option. Please try again.\n");
                break;
        }
        if(!ok){
            std::cout<<"Error adding breakpoint. Retrying..."<<std::endl;
        }
    }
}

static void del_breakpoint_menu(Debug_api& api){
    size_t index = 0;
    std::string br_choice;
    bool ok = false;
    while(!ok){
        std::cout<<"This are the current breakpoints:"<<std::endl;
        std::cout<<api.breakpoints_toString()<<std::endl;

        std::cout<<"Which type of breakpoint do you wish to delete?"<<std::endl;
        std::cout<<"p: Position"<<std::endl;
        std::cout<<"o: Opcode"<<std::endl;
        std::cout<<"m: Memory"<<std::endl;
        std::cout<<"r: Register"<<std::endl;
        std::cout<<"c: Clear all breakpoints"<<std::endl;
        std::cout<<"x: Cancel"<<std::endl;
        br_choice = read_line();
        Breakpoint_type type;
        const char* type_name;
        switch (br_choice[0]){
            case 'p': type = Breakpoint_type::POS;    type_name = "position"; break;
            case 'o': type = Breakpoint_type::OPCODE; type_name = "opcode";   break;
            case 'm': type = Breakpoint_type::MEM;    type_name = "memory";   break;
            case 'r': type = Breakpoint_type::REG;    type_name = "register"; break;
            case 'c':
                api.clear_breakpoints();
                std::cout<<"All breakpoints cleared."<<std::endl;
                return;
            case 'x':
                return;
            default:
                std::cout<<"Invalid option. Please try again."<<std::endl;
                continue;
        }
        std::cout<<"Enter the index of the "<<type_name<<" breakpoint you wish to delete: ";
        ok = parse_index(read_line(), index) && api.del_breakpoint(type, index);
        if(!ok){
            std::cout<<"Error deleting breakpoint. Retrying..."<<std::endl;
        }
    }
    std::cout<<"Breakpoint deleted succesfully"<<std::endl;
}

void platform_debug_menu(Debug_api& api){
    bool exit = false;
    while(!exit){
        printf("Enter your command (h for help): ");
        char command[25];
        fflush(stdin);
        if (fgets(command, sizeof(command), stdin) == nullptr)
            command[0] = '\0';
        command[strcspn(command, "\n")] = '\0';
        switch(command[0]){
            case 0: case 's':
                api.step();
                exit = true;
                continue;
                break;
            case 'b':
                add_breakpoint_menu(api);
                break;
            case 'd':
                std::cout<<api.breakpoints_toString()<<std::endl;
                break;
            case 'x':
                del_breakpoint_menu(api);
                break;
            case 'h':
                printf("h: Help\n");
                printf("b: Add breakpoint\n");
                printf("d: Display breakpoints\n");
                printf("x: Delete breakpoint\n");
                printf("g: Toggle debug windows\n");
                printf("p: Clear main screen\n");
                printf("s: Step\n");
                printf("c: Continue\n");
                printf("r: Reset\n");
                printf("m: Memory dump\n");
                printf("i: Debugger\n");
                printf("v: See last 10 PC values\n");
                printf("q: Quit\n");
                break;
            case 'g': {
                printf("Toggle debug windows:\n");
                printf("  1. Tile Viewer  [%s]\n", api.is_debug_window_active(DebugWindowType::TILES)   ? "ON" : "OFF");
                printf("  2. BG Map       [%s]\n", api.is_debug_window_active(DebugWindowType::BG_MAP)  ? "ON" : "OFF");
                printf("  3. Window Map   [%s]\n", api.is_debug_window_active(DebugWindowType::WIN_MAP) ? "ON" : "OFF");
                printf("  4. OAM Sprites  [%s]\n", api.is_debug_window_active(DebugWindowType::OAM)     ? "ON" : "OFF");
                printf("  0. Cancel\n");
                fflush(stdin);
                std::string choice = read_line();
                int idx = choice[0] - '1';
                if (idx >= 0 && idx < NUM_DEBUG_WINDOWS){
                    api.toggle_debug_window((DebugWindowType)idx);
                }
                break;
            }
            case 'p':
                api.clear_main_screen();
                break;
            case 'q':
                printf("Quitting...\n");
                exit = true;
                api.quit();
                break;
            case 'c':
                api.resume();
                exit = true;
                break;
            case 'r':
                api.reset();
                printf("Machine reset.\n");
                api.resume();
                exit = true;
                break;
            case 'm':
                api.dump_memory();
                break;
            case 'v':
                std::cout<<"[";
                for (u16 pc : api.last_pc_values()){
                    std::cout<<numToHexString(pc, 4)<<" ";
                }
                std::cout<<"]"<<std::endl;
                break;
            case 'i':
                std::cout<<"Opening VsCode debugger..."<<std::endl; //Place breakpoint here
                break;
        }
    }
}
