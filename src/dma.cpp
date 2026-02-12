#include <dma.h>
#include <memory.h>
Dma::Dma(Memory* mem) : mem(mem) {

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