#include "timer.h"
const u16 Timer::clock_table [4] = {256*4, 4*4, 16*4, 64*4}; //All 4 TIMA speeds
Timer::Timer(Cpu& cpu, Memory& mem) : cpu(cpu), mem(mem) {
    this->reset();
}
Tac_Struct Timer::get_tac(){ //Reads TAC and analyzes its bits to determine enabled and clock
    u8 tac = this->mem.readX(TAC_ADDR);
    Tac_Struct tac_struct;
    tac_struct.enabled = (bool)(tac & 0x04);
    tac_struct.clock_select = tac & 0x03;
    
    return tac_struct;
}
void Timer::reset(){ //Everything else is reset by mem
    this->tima_accumulation = 0;
}
void Timer::tick(){
    if(this->cpu.get_state() != STOPPED){ //When stopped, the timer stops working
        internal_clock++;
        if(internal_clock >=256){
            internal_clock = 0;
            this->mem.writeX(DIV_ADDR, (u16) ((this->mem.readX(DIV_ADDR) + 1) & 0xFF)); //Increment DIV
        }
        Tac_Struct tac = this->get_tac();
        if (tac.enabled){
            this->tima_accumulation++; //Clock_select determine how many timer ticks it takes to increment TIMA
            if(this->tima_accumulation >= this->clock_table[tac.clock_select]){
                this->tima_accumulation = 0;
                u8 tima = this->mem.readX(TIMA_ADDR);
                tima++;
                if(tima == 0){
                    this->trigger_on_next = true; //TIMA actually takes 1 more cycle to trigger the interrupt
                    return;
                }
                this->mem.writeX(TIMA_ADDR, tima);
            }
        }
    }
    else{
        this->mem.writeX(DIV_ADDR, (u16)0); //When stopped, DIV is always 0
        internal_clock = 0;
    }
    if (this->trigger_on_next){ //TIMA's actual int trig
        this->trigger_on_next = false;
        this->mem.writeX(TIMA_ADDR, this->mem.readX(TMA_ADDR)); //TIMA = TMA
        this->mem.writeX(IF_ADDR, (u16)(this->mem.readX(IF_ADDR) | 0x04)); //Adding timer to IF
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
    str += "DIV: " + numToHexString((u16)this->mem.readX(DIV_ADDR), 2) + " ";
    str += "TIMA: " + numToHexString((u16)this->mem.readX(TIMA_ADDR), 2) + " ";
    str += "TMA: " + numToHexString((u16)this->mem.readX(TMA_ADDR), 2) + " ";
    str += this->tac_toString();
    return str;
}
Timer_trace Timer::get_trace(){
    Timer_trace trace;
    trace.div = this->mem.readX(DIV_ADDR);
    trace.tima = this->mem.readX(TIMA_ADDR);
    trace.tma = this->mem.readX(TMA_ADDR);
    trace.tac = this->mem.readX(TAC_ADDR);
    return trace;
}

Timer_ss Timer::save_state() {
    Timer_ss state;
    state.div = this->mem.readX(DIV_ADDR);
    state.tima = this->mem.readX(TIMA_ADDR);
    state.tma = this->mem.readX(TMA_ADDR);
    state.tac = this->mem.readX(TAC_ADDR);
    state.tima_accumulation = this->tima_accumulation;
    state.internal_clock = this->internal_clock;
    return state;
}

void Timer::load_state(const Timer_ss& state) {
    this->mem.writeX(DIV_ADDR, state.div);
    this->mem.writeX(TIMA_ADDR, state.tima);
    this->mem.writeX(TMA_ADDR, state.tma);
    this->mem.writeX(TAC_ADDR, state.tac);
    this->tima_accumulation = state.tima_accumulation;
    this->internal_clock = state.internal_clock;
}