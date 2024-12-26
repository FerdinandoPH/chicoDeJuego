#include <emu.h>
#include <memory.h>
#include <utils.h>
#include <cpu.h>
#include <timer.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>

Debug_mode dbg_level = FULL_DBG;
Memory* memory = new Memory();
Cpu* cpu = new Cpu(*memory);
Timer* timer = new Timer(*cpu, *memory);
int ticks = 0;
void delay(u32 ms){
    SDL_Delay(ms);
}
void emu_reset(){
    ticks = 0;
    memory->reset();
    cpu->reset();
    timer->reset();
}
void debug_print(){
    std::cout<<"Ticks: "<<ticks<<std::endl;
    std::cout<<timer->toString()<<std::endl;
    std::cout<<cpu->toString()<<std::endl;
}
int emu_run(int argc, char** argv){
    
    if(argc < 2 || !memory->load_rom(argv[1])){
        printf("Error loading ROM\n");
        return 1;
    }
    printf("ROM loaded: %s\n\n", memory->rom_header->title);
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    
    while(cpu->state != QUIT){
        cpu->check_interrupts();
        if(cpu->state == PAUSED){
            delay(10);
            continue;
        }
        if(dbg_level == PRINT_DBG || dbg_level == FULL_DBG){
            debug_print();
            if(dbg_level == FULL_DBG){
                bool exit = false;
                while(!exit){
                    printf("Enter your command (h for help): ");
                    char command[25];
                    fgets(command, sizeof(command), stdin);
                    command[strcspn(command, "\n")] = '\0';
                    switch(command[0]){
                        case 0: case 's':
                            exit = true;
                            continue;
                            break;
                        case 'h':
                            printf("h: Help\n");
                            printf("q: Quit\n");
                            printf("s: Step\n");
                            printf("c: Continue\n");
                            printf("r: Reset\n");
                            printf("m: Memory dump\n");
                            printf("i: Debugger\n");
                            break;
                        case 'q':
                            printf("Quitting...\n");
                            exit = true;
                            cpu->state = QUIT;
                            break;
                        case 'c':
                            cpu->state = RUNNING;
                            break;
                        case 'r':
                            printf("Resetting...\n");
                            emu_reset();
                            debug_print();
                            break;
                        case 'm':
                            memory->dump();
                            break;
                        case 'i':
                            std::cout<<"Opening VsCode debugger..."<<std::endl; //Place breakpoint here
                            break;
                    }
                }
                printf("\n");
            }
        }
        cpu->step();

    }
    return 0;
}
void run_ticks(int ticks_to_run){
    for(int i=0; i<ticks_to_run; i++){
        ticks++;
        timer->tick();
    }
}

