#include "dma.h"
#include "memory.h"
Dma::Dma(Memory* mem) : mem(mem) {

}
void Dma::reset(){
    this->source = 0;
    this->dest = 0;
    this->bytes_left = 0;
    this->transferring = false;
}
void Dma::start(u16 source, u16 dest, u16 length){
    this->source = source;
    this->dest = dest;
    this->bytes_left = length;
    this->transferring = true;
}
void Dma::tick(){
    if(transferring){
        mem->writeX(dest, mem->readX(source));
        source++;
        dest++;
        bytes_left--;
        if(bytes_left == 0){
            transferring = false;
        }
    }
}

Dma_ss Dma::save_state() {
    Dma_ss state;
    state.source = this->source;
    state.dest = this->dest;
    state.bytes_left = this->bytes_left;
    state.transferring = this->transferring;
    return state;
}

void Dma::load_state(const Dma_ss& state) {
    this->source = state.source;
    this->dest = state.dest;
    this->bytes_left = state.bytes_left;
    this->transferring = state.transferring;
}

Vdma_ss Vdma::save_state() {
    Vdma_ss state;
    state.source = this->source;
    state.dest = this->dest;
    state.bytes_left = this->bytes_left;
    state.bytes_transferred_in_round = this->bytes_transferred_in_round;
    state.mode = this->mode;
    state.state = this->state;
    return state;
}

void Vdma::load_state(const Vdma_ss& state) {
    this->source = state.source;
    this->dest = state.dest;
    this->bytes_left = state.bytes_left;
    this->bytes_transferred_in_round = state.bytes_transferred_in_round;
    this->mode = state.mode;
    this->state = state.state;
}