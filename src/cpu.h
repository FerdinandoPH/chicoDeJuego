#pragma once;

typedef enum {RUN, PAUSE, STOP, HALT, QUIT} Cpu_State;

class Memory;
class Cpu{
    private:
        Memory* mem;
    public:
        Cpu_State state = RUN;
        int ticks = 0;
        
        Cpu(Memory* mem);
        ~Cpu();
        bool step();
};