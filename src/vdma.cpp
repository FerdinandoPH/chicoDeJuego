#include "dma.h"
#include "memory.h"
#include "ppu.h"
#include "cpu.h"

Vdma::Vdma(Memory* mem, Ppu* ppu, Cpu* cpu) : mem(mem), ppu(ppu), cpu(cpu), source(0), dest(0), bytes_left(0), mode(Vdma_mode::GENERAL) {}

void Vdma::set_src(u16 addr, u8 data){
    if(addr== VDMA1_ADDR){
        this->source = (this->source & 0x00FF) | (data << 8);
    }
    else if(addr==VDMA2_ADDR){
        this->source = (this->source & 0xFF00) | (data & 0xF0);
    }
}

void Vdma::set_dest(u16 addr, u8 data){
    if(addr== VDMA3_ADDR){
        this->dest = (this->dest & 0x00FF) | (((data & 0x1F) | 0x80) << 8);
    }
    else if(addr==VDMA4_ADDR){
        this->dest = (this->dest & 0xFF00) | (data & 0xF0);
    }
}

u8 Vdma::set_VDMA5(u8 data){
    if(this->state == Vdma_state::IDLE){
        this->bytes_left = ((data & 0x7F) + 1) * 0x10;
        this->mode = (data & 0x80) ? Vdma_mode::HBLANK : Vdma_mode::GENERAL;
        if(this->mode == Vdma_mode::GENERAL){
            cpu->set_state(Cpu_State::VDMA_HALTED);
            this->state = Vdma_state::TRANSFERRING;
        }
        else{
            this->state = Vdma_state::WAITING_FOR_HBLANK;
        }
        return data & 0x7F;
    }else{
        if(data & 0x80){
            this->state = Vdma_state::IDLE;
            return 0x80 | ((this->bytes_left / 0x10) - 1);
        }else{
            bytes_left = ((data & 0x7F) + 1) * 0x10;
            return data & 0x7F;
        }
    }

}

void Vdma::tick(){
    switch(this->state){
        case Vdma_state::IDLE:
            break;
        case Vdma_state::TRANSFERRING:
            if(this->bytes_left > 0){
                u8 data = this->mem->readX(this->source);
                this->mem->writeX(this->dest, data);
                this->source++;
                this->dest = (this->dest + 1) & 0x9FFF; //Wrap around to 0x8000 if it goes over 0x9FFF
                this->bytes_left--;
                if (bytes_left % 0x10 == 0) {
                    mem->writeX(VDMA5_ADDR, (u8)((this->bytes_left / 0x10) - 1));
                }
                if(this->mode == Vdma_mode::HBLANK){
                    this->bytes_transferred_in_round++;
                    if(this->bytes_transferred_in_round == 0x10){
                        this->bytes_transferred_in_round = 0;
                        this->state = Vdma_state::WAITING_FOR_NEXT_LINE;
                        cpu->set_state(Cpu_State::RUNNING);
                    }
                }
            }
            else{
                this->source = 0xFFFF;
                this->dest = 0xFFFF;
                mem->writeX(VDMA5_ADDR, (u8)0xFF);
                this->state = Vdma_state::IDLE;
                cpu->set_state(Cpu_State::RUNNING);
            }
            break;
        case Vdma_state::WAITING_FOR_HBLANK:
            if(this->ppu->get_mode() == Ppu_mode::HBLANK && cpu->get_state() == Cpu_State::RUNNING){
                this->state = Vdma_state::TRANSFERRING;
                cpu->set_state(Cpu_State::VDMA_HALTED);
            }
            break;
        case Vdma_state::WAITING_FOR_NEXT_LINE:
            if(this->ppu->get_mode() != Ppu_mode::HBLANK){
                this->state = Vdma_state::WAITING_FOR_HBLANK;
            }
            break;
    }
}