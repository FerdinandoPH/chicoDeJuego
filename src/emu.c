#include <stdio.h>
#include <emu.h>
#include <cart.h>
#include <mem.h>
#include <cpu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

static EmuData emuData;
Memory mem;
DebugMode dbg_level = FULL;
void delay(u32 ms){
    SDL_Delay(ms);
}

int emuRun(int argc, char **argv){
    if (argc < 2){
        printf("No ROM found\n");
        return 2;
    }
    if (!cartLoad(argv[1], &mem, 0x0, 0x8000)){
        printf("Error loading ROM\n");
        return 1;
    }

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    cpuInit();
    emuData.state=RUN;
    emuData.ticks=0;

    while(emuData.state != QUIT){
        if (emuData.state == PAUSE){
            delay(10);
            continue;
        }
        if (dbg_level == PRINT || dbg_level == FULL){
            printf("Ticks: %lld\n", emuData.ticks);
            if (dbg_level == FULL){
                bool exit = false;
                while (!exit){
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
                            printf("i: C Debugger\n");
                            break;
                        case 'q':
                            exit = true;
                            emuData.state=QUIT;
                            printf("Quitting...\n");
                            break;
                        case 'c':
                            dbg_level = NONE;
                            exit = true;
                            break;
                        case 'm':
                            memDump(&mem);
                            break;
                        case 'i':
                            break;
                    }
                }
            }
        }
        if(!cpuStep()){
            printf("CPU error\n");
            return 3;
        }
        emuData.ticks++;
    }


    return 0;
}