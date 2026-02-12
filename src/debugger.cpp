#include <debugger.h>


std::unordered_map<std::string, Dbg_cond> dbg_cond_map = {
    {"==", Dbg_cond::EQ}, {"!=", Dbg_cond::NEQ}, {">", Dbg_cond::GT}, {"<", Dbg_cond::LT}, {">=", Dbg_cond::GTE}, {"<=", Dbg_cond::LTE}, {"><", Dbg_cond::BET}
};
std::unordered_map<Dbg_cond, std::string> dbg_cond_names = {
    {Dbg_cond::EQ, "=="}, {Dbg_cond::NEQ, "!="}, {Dbg_cond::GT, ">"}, {Dbg_cond::LT, "<"}, {Dbg_cond::GTE, ">="}, {Dbg_cond::LTE, "<="}, {Dbg_cond::BET, "><"}
};
Debugger::Debugger(int& ticks, Memory& mem, Cpu& cpu, Timer& timer, Ppu& ppu) : ticks(ticks), mem(mem), cpu(cpu), timer(timer), ppu(ppu){
    this->dbg_level = FULL_DBG;
    this->start_chrono();
    this->last_pc_values = std::vector<u16>(10, 0x100);
}


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
                    ok = this->add_reg_breakpoint(reg, cond, value);
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