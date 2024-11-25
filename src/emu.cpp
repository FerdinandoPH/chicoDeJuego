#include <emu.h>
#include <memory.h>
#include <utils.h>
#include <cpu.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
Debug_mode dbg_level = FULL_DBG;
int emu_run(int argc, char** argv){
    Memory* memory = new Memory();
    if(argc < 2 || !memory->load_rom(argv[1])){
        printf("Error loading ROM\n");
        return 1;
    }
    printf("ROM loaded: %s\n", memory->rom_header->title);
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    Cpu* cpu = new Cpu(memory);
    while(cpu->state != QUIT){
        
    }
    return 0;
}