#include <timer.h>
Timer::Timer(Cpu& cpu, Memory& mem) : cpu(cpu), mem(mem) {

}
Tac_Struct Timer::get_tac(){ //Reads TAC (0xFF07) and analyzes its bits to determine enabled and clock
    u8 tac = this->mem.readX(0xFF07);
    Tac_Struct tac_struct;
    tac_struct.enabled = (bool)(tac & 0x04);
    tac_struct.clock_select = tac & 0x03;
    
    return tac_struct;
}
void Timer::reset(){ //Everything else is reset by mem
    this->tima_accumulation = 0;
}
void Timer::tick(){
    if(this->cpu.state != STOPPED){ //When stopped, the timer stops working
        this->mem.writeX((u16)0xFF04, (u16) ((this->mem.readX(0xFF04) + 1) & 0xFF)); //Increment DIV
        Tac_Struct tac = this->get_tac();
        if (tac.enabled){
            this->tima_accumulation++; //Clock_select determine how many timer ticks it takes to increment TIMA
            if(this->tima_accumulation >= this->clock_table[tac.clock_select]){
                this->tima_accumulation = 0;
                u8 tima = this->mem.readX(0xFF05);
                tima++;
                if(tima == 0){
                    this->trigger_on_next = true; //TIMA actually takes 1 more cycle to trigger the interrupt
                    return;
                }
                this->mem.writeX(0xFF05, tima);
            }
        }
    }
    else{
        this->mem.writeX((u16)0xFF04, (u16)0); //When stopped, DIV is always 0
    }
    if (this->trigger_on_next){ //TIMA's actual int trig
        this->trigger_on_next = false;
        this->mem.writeX(0xFF05, this->mem.readX(0xFF06)); //TIMA = TMA
        this->mem.writeX((u16)0xFF0F, (u16)(this->mem.readX(0xFF0F) | 0x04)); //Adding timer to IF
    }
}
std::string Timer::tac_toString(){
    Tac_Struct tac = this->get_tac();
    std::string str = "TAC: ";
    str += "Enabled: " + std::to_string(tac.enabled) + " ";
    str += "Clock select: " + std::to_string(this->clock_table[tac.clock_select]) + " ";
    return str;
}
std::string Timer::toString(){
    std::string str = "Timer: ";
    str += "DIV: " + numToHexString((u16)this->mem.readX(0xFF04), 2) + " ";
    str += "TIMA: " + numToHexString((u16)this->mem.readX(0xFF05), 2) + " ";
    str += "TMA: " + numToHexString((u16)this->mem.readX(0xFF06), 2) + " ";
    str += this->tac_toString();
    return str;
}