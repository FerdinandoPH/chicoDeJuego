#include <emu.h>
#include <debugger.h>
#include <memory.h>
#include <utils.h>
#include <cpu.h>
#include <timer.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <csignal>

Memory* memory = new Memory();
Cpu* cpu = new Cpu(*memory);
Timer* timer = new Timer(*cpu, *memory);
int ticks = 0;
Debugger dbg = Debugger(ticks, *memory, *cpu, *timer);
void signal_handler(int signal){
    if (signal == SIGINT){
        if (dbg.dbg_level != FULL_DBG){
            std::cout << "SIGINT detected"<<std::endl<<std::endl;
            dbg.dbg_level = FULL_DBG;
        }
        else{
            std::signal(SIGINT, SIG_DFL);
            std::raise(SIGINT);
        }
    }
}
void delay(u32 ms){
    SDL_Delay(ms);
}
void emu_reset(){
    ticks = 0;
    memory->reset();
    cpu->reset();
    timer->reset();
    dbg.clear_breakpoints();
}

int emu_run(int argc, char** argv){
    std::signal(SIGINT, signal_handler);

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
        
        if(dbg.dbg_level != NO_DBG){
            dbg.check_breakpoints();
            if(dbg.dbg_level == PRINT_DBG || dbg.dbg_level == FULL_DBG){
                dbg.debug_print();
                if(dbg.dbg_level == FULL_DBG){
                    bool exit = false;
                    while(!exit){
                        printf("Enter your command (h for help): ");
                        char command[25];
                        fflush(stdin);
                        fgets(command, sizeof(command), stdin);
                        command[strcspn(command, "\n")] = '\0';
                        switch(command[0]){
                            case 0: case 's':
                                exit = true;
                                continue;
                                break;
                            case 'b':
                                dbg.add_breakpoint_menu();
                                break;
                            case 'd':
                                std::cout<<dbg.breakpoints_toString()<<std::endl;
                                break;
                            case 'x':
                                dbg.del_breakpoint_menu();
                                break;
                            case 'h':
                                printf("h: Help\n");
                                printf("b: Add breakpoint\n");
                                printf("d: Display breakpoints\n");
                                printf("x: Delete breakpoint");
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
                                dbg.dbg_level = OFF_DBG;
                                exit = true;
                                break;
                            case 'r':
                                printf("Resetting...\n");
                                emu_reset();
                                dbg.debug_print();
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



