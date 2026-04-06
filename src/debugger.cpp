#include <debugger.h>


std::unordered_map<std::string, Dbg_cond> dbg_cond_map = {
    {"==", Dbg_cond::EQ}, {"!=", Dbg_cond::NEQ}, {">", Dbg_cond::GT}, {"<", Dbg_cond::LT}, {">=", Dbg_cond::GTE}, {"<=", Dbg_cond::LTE}, {"><", Dbg_cond::BET}
};
std::unordered_map<Dbg_cond, std::string> dbg_cond_names = {
    {Dbg_cond::EQ, "=="}, {Dbg_cond::NEQ, "!="}, {Dbg_cond::GT, ">"}, {Dbg_cond::LT, "<"}, {Dbg_cond::GTE, ">="}, {Dbg_cond::LTE, "<="}, {Dbg_cond::BET, "><"}
};
std::unordered_map<u16, std::string> interrupt_names = {
    {0x40, "V-Blank"}, {0x48, "LCD STAT"}, {0x50, "Timer"}, {0x58, "Serial"}, {0x60, "Joypad"}
};
Debugger::Debugger(Debug_mode initial_dbg_level, int& ticks, Memory& mem, Cpu& cpu, Timer& timer, Ppu& ppu) : ticks(ticks), mem(mem), cpu(cpu), timer(timer), ppu(ppu){
    this->dbg_level = initial_dbg_level;
    this->start_chrono();
    this->last_pc_values = std::vector<u16>(10, 0x100);
}
#ifdef TRACEGEN
void Debugger::generate_trace_header(){
    this->trace_file = fopen("chicoDeJuego.jsonl", "w");
    fprintf(this->trace_file, "{\"_header\":true,\"format_version\":\"0.1.0\",\"emulator\":\"chicoDeJuego\",\"emulator_version\":\"0.1\",\"rom_sha256\":\"%s\",\"model\":\"DMG-B\",\"boot_rom\":\"skip\",\"profile\":\"blargg\",\"fields\":[\"pc\",\"sp\",\"a\",\"f\",\"b\",\"c\",\"d\",\"e\",\"h\",\"l\",\"lcdc\",\"stat\",\"ly\",\"lyc\",\"scy\",\"scx\",\"if_\",\"ie\",\"ime\",\"div\",\"tima\",\"tma\",\"tac\"],\"trigger\":\"instruction\"}", this->mem.get_sha256().c_str());
}
Debugger::~Debugger(){
    if (this->trace_file != nullptr){
        fclose(this->trace_file);
    }
}
#endif


void Debugger::debug_print(){
    std::cout<<"Ticks: "<<this->ticks<<std::endl;
    std::cout<<this->timer.toString()<<std::endl;
    std::cout<<this->ppu.toString()<<std::endl;
    std::cout<<this->cpu.toString()<<std::endl;
}

bool Debugger::add_pos_breakpoint(std::string pos){
    try{
        u16 pos_num = std::stoi(pos, nullptr, 16);
        this->pos_breakpoints.push_back(pos_num);
        std::cout<<"Added pos breakpoint at "<<numToHexString(pos_num, 4)<<std::endl;
        return true;
    }
    catch(...){
        return false;
    }
}

bool Debugger::add_opcode_breakpoint(std::string opcode){
    try{
        u8 opcode_num = std::stoi(opcode, nullptr, 16);
        this->opcode_breakpoints.push_back(opcode_num);
        std::cout<<"Added opcode breakpoint at "<<numToHexString(opcode_num, 2)<<std::endl;
        return true;
    }
    catch(...){
        return false;
    }
}

bool Debugger::add_mem_breakpoint(std::string addr, std::string cond, std::string value, std::string value2){
    try{
        Mem_breakpoint bp;
        bp.addr = std::stoi(addr, nullptr, 16);
        bp.cond = dbg_cond_map.at(cond);
        if (value == "X" || value == "x"){
            bp.value = mem.readX(bp.addr);
            bp.current = true;
        }
        else{
            bp.value = std::stoi(value, nullptr, 16);
        }
        if (!value2.empty()){
            bp.value2 = std::stoi(value2, nullptr, 16);
        }
        this->mem_breakpoints.push_back(bp);
        std::cout<<"Added mem breakpoint at "<<numToHexString(bp.addr, 4)<<" with condition "<<dbg_cond_names[bp.cond]<<" and value "<<numToHexString(bp.value, 2)<<(!value2.empty()?("<->"+numToHexString(bp.value2, 2)):"")<<std::endl;
        return true;
    }
    catch(...){
        return false;
    }
}
bool Debugger::add_reg_breakpoint(std::string reg, std::string cond, std::string value, std::string value2){
    try{
        Reg_breakpoint bp;
        std::transform(reg.begin(), reg.end(), reg.begin(), ::toupper);
        bp.reg = reg_map.at(reg);
        bp.cond = dbg_cond_map.at(cond);
        if (value == "X" || value == "x"){
            bp.value = this->cpu.regs[bp.reg];
            bp.current = true;
        }
        else{
            bp.value = std::stoi(value, nullptr, 16);
        }
        if (!value2.empty()){
            bp.value2 = std::stoi(value2, nullptr, 16);
        }
        this->reg_breakpoints.push_back(bp);
        std::cout<<"Added reg breakpoint at "<<reg_names[bp.reg]<<" with condition "<<dbg_cond_names[bp.cond]<<" and value "<<numToHexString(bp.value)<<(!value2.empty()?("<->"+numToHexString(bp.value2)):"")<<std::endl;
        return true;
    }
    catch(...){
        return false;
    }
}
bool Debugger::del_breakpoint(Breakpoint_type br_type, size_t index){
    index--;
    switch(br_type){
        case Breakpoint_type::POS:
            if (index < this->pos_breakpoints.size()){
                this->pos_breakpoints.erase(this->pos_breakpoints.begin() + index);
                return true;
            }
            break;
        case Breakpoint_type::OPCODE:
            if (index < this->opcode_breakpoints.size()){
                this->opcode_breakpoints.erase(this->opcode_breakpoints.begin() + index);
                return true;
            }
            break;
        case Breakpoint_type::MEM:
            if (index < this->mem_breakpoints.size()){
                this->mem_breakpoints.erase(this->mem_breakpoints.begin() + index);
                return true;
            }
            break;
        case Breakpoint_type::REG:
            if (index < this->reg_breakpoints.size()){
                this->reg_breakpoints.erase(this->reg_breakpoints.begin() + index);
                return true;
            }
            break;
    }
    return false;
}
void Debugger::clear_breakpoints(){
    this->pos_breakpoints.clear();
    this->opcode_breakpoints.clear();
    this->mem_breakpoints.clear();
    this->reg_breakpoints.clear();
}
bool Debugger::check_breakpoints(){
    if(cpu.regs[PC] != this->last_pc_values[0]){
        last_pc_values.insert(last_pc_values.begin(), cpu.regs[PC]);
        last_pc_values.pop_back();
    }
    for (auto it = this->pos_breakpoints.begin(); it != this->pos_breakpoints.end(); it++){
        if (*it == this->cpu.regs[PC]){
            std::cout<<"Reached pos breakpoint at "<<numToHexString(*it, 4)<<std::endl;
            this->dbg_level = FULL_DBG;
            return true;
        }
    }
    for (auto it = this->opcode_breakpoints.begin(); it != this->opcode_breakpoints.end(); it++){
        if (*it == this->mem.readX(this->cpu.regs[PC])){
            std::cout<<"Reached opcode breakpoint at "<<numToHexString(*it, 2)<<std::endl;
            this->dbg_level = FULL_DBG;
            return true;
        }
    }
    for (auto it = this->mem_breakpoints.begin(); it != this->mem_breakpoints.end(); it++){
        u8 value = this->mem.readX(it->addr);
        if (check_dbg_cond(it->cond, value, it->value, it->value2)){
            std::cout<<"Reached mem breakpoint at "<<numToHexString(it->addr, 4)<<": "<<numToHexString(it->value, 2)<<" "<<dbg_cond_names[it->cond]<<" "<<numToHexString(value, 2)<<std::endl;
            if (it->current && it->cond == Dbg_cond::NEQ){
                it->value = value;
            }
            this->dbg_level = FULL_DBG;
            return true;
        }
    }
    for (auto it = this->reg_breakpoints.begin(); it != this->reg_breakpoints.end(); it++){
        u16 value = this->cpu.regs[it->reg];
        if (check_dbg_cond(it->cond, value, it->value, it->value2)){
            std::cout<<"Reached reg breakpoint at "<<reg_names[it->reg]<<": "<<numToHexString(it->value, 2)<<" "<<dbg_cond_names[it->cond]<<" "<<numToHexString(value, 2)<<std::endl;
            if (it->current && it->cond == Dbg_cond::NEQ){
                it->value = value;
            }
            this->dbg_level = FULL_DBG;
            return true;
        }
    }
    return false;
}
std::string Debugger::breakpoints_toString(){
    std::string str = "";
    int index = 1;
    str += "Pos breakpoints:\n";
    for (auto it = this->pos_breakpoints.begin(); it != this->pos_breakpoints.end(); it++){
        str += std::to_string(index++) + ": " + numToHexString(*it, 4) + " ";
    }
    index = 1;
    str += "\nOpcode breakpoints:\n";
    for (auto it = this->opcode_breakpoints.begin(); it != this->opcode_breakpoints.end(); ++it) {
        str += std::to_string(index++) + ": " + numToHexString(*it, 2) + " ";
    }

    index = 1;
    str += "\nMem breakpoints:\n";
    for (auto it = this->mem_breakpoints.begin(); it != this->mem_breakpoints.end(); ++it) {
        str += std::to_string(index++) + ": " + numToHexString(it->addr, 4) + " " + dbg_cond_names[it->cond] + " " + numToHexString(it->value, 2) + " ";
    }

    index = 1;
    str += "\nReg breakpoints:\n";
    for (auto it = this->reg_breakpoints.begin(); it != this->reg_breakpoints.end(); ++it) {
        str += std::to_string(index++) + ": " + reg_names[it->reg] + " " + dbg_cond_names[it->cond] + " " + numToHexString(it->value, 2) + " ";
    }
    return str;
}
bool Debugger::check_dbg_cond(Dbg_cond cond, u16 val1, u16 val2, u16 val3){
    switch(cond){
        case Dbg_cond::EQ:
            return val1 == val2;
        case Dbg_cond::NEQ:
            return val1 != val2;
        case Dbg_cond::GT:
            return val1 > val2;
        case Dbg_cond::LT:
            return val1 < val2;
        case Dbg_cond::GTE:
            return val1 >= val2;
        case Dbg_cond::LTE:
            return val1 <= val2;
        case Dbg_cond::BET:
            return val1 >= val2 && val1 <= val3;
    }
    return false;
}
void Debugger::add_breakpoint_menu(){
    bool ok = false;
    while (!ok){
        printf("Enter breakpoint type:\n");
        printf("p: Position\n");
        printf("o: Opcode\n");
        printf("m: Memory\n");
        printf("r: Register\n");
        printf("x: Cancel\n");
        std::string type = "";
        std::string pos, addr, value, value2, opcode, cond, reg;
        std::getline(std::cin, type);
        switch(type[0]){
            case 'p':
                printf("Enter position: ");
                std::getline(std::cin, pos);
                ok = this->add_pos_breakpoint(pos);
                break;
            case 'o':
                printf("Enter opcode: ");
                std::getline(std::cin, opcode);
                ok = this->add_opcode_breakpoint(opcode);
                break;
            case 'm':
                printf("Enter address: ");
                std::getline(std::cin, addr);
                printf("Enter condition (==, !=, >, <, >=, <=, ><): ");
                std::getline(std::cin, cond);
                printf("Enter value: ");
                std::getline(std::cin, value);
                if(cond == "><"){
                    printf("Enter second value: ");
                    std::getline(std::cin, value2);
                    ok = this->add_mem_breakpoint(addr, cond, value, value2);
                }
                else{
                    ok = this->add_mem_breakpoint(addr, cond, value, "");
                }
                break;
            case 'r':
                printf("Enter register: ");
                std::getline(std::cin, reg);
                printf("Enter condition (==, !=, >, <, >=, <=, ><): ");
                std::getline(std::cin, cond);
                printf("Enter value: ");
                std::getline(std::cin, value);
                if (cond == "><"){
                    printf("Enter second value: ");
                    std::getline(std::cin, value2);
                    ok = this->add_reg_breakpoint(reg, cond, value, value2);
                }
                else
                    ok = this->add_reg_breakpoint(reg, cond, value, "");
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
void Debugger::del_breakpoint_menu(){
    int index = 0;
    std::string br_choice;
    bool ok = false;
    while(!ok){
        std::cout<<"This are the current breakpoints:"<<std::endl;
        std::cout<<this->breakpoints_toString()<<std::endl;
        
        std::cout<<"Which type of breakpoint do you wish to delete?"<<std::endl;
        std::cout<<"p: Position"<<std::endl;
        std::cout<<"o: Opcode"<<std::endl;
        std::cout<<"m: Memory"<<std::endl;
        std::cout<<"r: Register"<<std::endl;
        std::cout<<"c: Clear all breakpoints"<<std::endl;
        std::cout<<"x: Cancel"<<std::endl;
        std::getline(std::cin, br_choice);
        switch (br_choice[0]){
            case 'p':
                std::cout<<"Enter the index of the position breakpoint you wish to delete: ";
                std::getline(std::cin, br_choice);
                index = std::stoi(br_choice); //Cuidado, cambiado por copilot
                ok = this->del_breakpoint(Breakpoint_type::POS, index);
                break;
            case 'o':
                std::cout<<"Enter the index of the opcode breakpoint you wish to delete: ";
                std::getline(std::cin, br_choice);
                index = std::stoi(br_choice);
                ok = this->del_breakpoint(Breakpoint_type::OPCODE, index);
                break;
            case 'm':
                std::cout<<"Enter the index of the memory breakpoint you wish to delete: ";
                std::getline(std::cin, br_choice);
                index = std::stoi(br_choice);
                ok = this->del_breakpoint(Breakpoint_type::MEM, index);
                break;
            case 'r':
                std::cout<<"Enter the index of the register breakpoint you wish to delete: ";
                std::getline(std::cin, br_choice);
                index = std::stoi(br_choice);
                ok = this->del_breakpoint(Breakpoint_type::REG, index);
                break;
            case 'c':
                this->clear_breakpoints();
                std::cout<<"All breakpoints cleared."<<std::endl;
                ok = true;
                return;
            case 'x':
                ok = true;
                return;
            default:
                std::cout<<"Invalid option. Please try again."<<std::endl;
                break;
        }
        if(!ok){
            std::cout<<"Error deleting breakpoint. Retrying..."<<std::endl;
        }
    }
    std::cout<<"Breakpoint deleted succesfully"<<std::endl;
}

void Debugger::reset(){
    this->clear_breakpoints();
    this->dbg_level = FULL_DBG;
    this->last_pc_values = std::vector<u16>(10, 0x100);
}

void Debugger::start_chrono(){
    this->last_time = std::chrono::high_resolution_clock::now();
}

std::chrono::duration<double, std::micro> Debugger::get_chrono(){
    return std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(
        std::chrono::high_resolution_clock::now() - this->last_time
    );
}

#ifdef TRACEGEN
void Debugger::generate_trace(){
    Cpu_trace cpu_trace = this->cpu.get_trace();
    Timer_trace timer_trace = this->timer.get_trace();
    Ppu_trace ppu_trace = this->ppu.get_trace();
    // fprintf(this->trace_file, "\n{\"pc\":\"%u\",\"sp\":\"%u\",\"a\":\"%u\",\"f\":\"%u\",\"b\":\"%u\",\"c\":\"%u\",\"d\":\"%u\",\"e\":\"%u\",\"h\":\"%u\",\"l\":\"%u\",\"lcdc\":\"%u\",\"stat\":\"%u\",\"ly\":\"%u\",\"lyc\":\"%u\",\"scy\":\"%u\",\"scx\":\"%u\",\"if_\":\"%u\",\"ie\":\"%u\",\"ime\":%s,\"div\":\"%u\",\"tima\":\"%u\",\"tma\":\"%u\",\"tac\":\"%u\"}",
    //     cpu_trace.pc, cpu_trace.sp, cpu_trace.a, cpu_trace.f, cpu_trace.b, cpu_trace.c, cpu_trace.d, cpu_trace.e, cpu_trace.h, cpu_trace.l,
    //     ppu_trace.lcdc, ppu_trace.stat, ppu_trace.ly, ppu_trace.lyc, ppu_trace.scy, ppu_trace.scx,
    //     cpu_trace.IF, cpu_trace.IE, cpu_trace.IME ? "true" : "false",
    //     timer_trace.div, timer_trace.tima, timer_trace.tma, timer_trace.tac
    // );
    fprintf(
        this->trace_file,
        "\n{\"pc\":\"%04X\",\"sp\":\"%04X\",\"a\":\"%02X\",\"f\":\"%02X\",\"b\":\"%02X\",\"c\":\"%02X\",\"d\":\"%02X\",\"e\":\"%02X\",\"h\":\"%02X\",\"l\":\"%02X\",\"lcdc\":\"%02X\",\"stat\":\"%02X\",\"ly\":\"%02X\",\"lyc\":\"%02X\",\"scy\":\"%02X\",\"scx\":\"%02X\",\"if_\":\"%02X\",\"ie\":\"%02X\",\"ime\":%s,\"div\":\"%04X\",\"tima\":\"%02X\",\"tma\":\"%02X\",\"tac\":\"%02X\"}",
        cpu_trace.pc, cpu_trace.sp, cpu_trace.a, cpu_trace.f, cpu_trace.b, cpu_trace.c, cpu_trace.d, cpu_trace.e, cpu_trace.h, cpu_trace.l,
        ppu_trace.lcdc, ppu_trace.stat, ppu_trace.ly, ppu_trace.lyc, ppu_trace.scy, ppu_trace.scx,
        cpu_trace.IF, cpu_trace.IE, cpu_trace.IME ? "true" : "false",
        timer_trace.div, timer_trace.tima, timer_trace.tma, timer_trace.tac
    );
}
#endif