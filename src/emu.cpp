#include <emu.h>
#include <debugger.h>
#include <memory.h>
#include <utils.h>
#include <cpu.h>
#include <timer.h>
#include <stdio.h>
#include <ui.h>
#include <ppu.h>
#include <dma.h>
#include <controller.h>
#include <pthread.h>
#include <iostream>
#include <csignal>
#include <mutex>
#include <chrono>

Debug_mode initial_dbg_mode = NO_DBG;

Memory* memory = new Memory();
Controller* controller = new Controller(*memory);
Dma* dma = new Dma(memory);
Cpu* cpu = new Cpu(*memory);
Timer* timer = new Timer(*cpu, *memory);
int ticks = 0;
bool resetting = false;
Ui* ui = new Ui(*memory, *controller, 4);
Ppu* ppu = new Ppu(*memory, ui, 4);
std::mutex ui_mutex = std::mutex();
Debugger dbg = Debugger(initial_dbg_mode, ticks, *memory, *cpu, *timer, *ppu);

void signal_handler(int signal){
    if (signal == SIGINT){
        std::signal(SIGINT, signal_handler);
        if (dbg.dbg_level != FULL_DBG){
            std::cout << "SIGINT detected"<<std::endl<<std::endl;
            dbg.dbg_level = FULL_DBG;
        }
        else{
            cpu->state = QUIT;
            std::exit(1);
        }
    }
}
void delay(u32 ms){
    SDL_Delay(ms);
}
void emu_reset(std::binary_semaphore* sem = nullptr){
    if (sem != nullptr){
        resetting = true;
        sem->acquire();
    }
    ticks = 0;
    memory->reset();
    cpu->reset();
    timer->reset();
    ppu->reset();
    controller->reset();
    dbg.reset();
}
void cpu_run(void* thread_args){
    std::binary_semaphore* sem = ((Cpu_thread_args*)thread_args)->sem;
    //std::chrono::duration<double, std::micro> elapsed = dbg.get_chrono();
    //FILE* log_pc = fopen("chicoDeJuego.emulog", "wb");
    while(cpu->state != QUIT){
        if(cpu->check_interrupts() && (dbg.dbg_level == FULL_DBG || dbg.dbg_level == PRINT_DBG)){
            printf("%s interrupt triggered\n",interrupt_names[cpu->regs[PC]].c_str());
        }
        if(cpu->state == PAUSED){
            delay(10);
            continue;
        }
        #ifdef TRACEGEN
        dbg.generate_trace();
        #endif
        if(dbg.dbg_level != NO_DBG){
            
            dbg.check_breakpoints();
            
            if(dbg.dbg_level == PRINT_DBG || dbg.dbg_level == FULL_DBG){
                
                //std::cout<<"Elapsed time: "<<elapsed.count()<<"us"<<std::endl;
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
                                printf("g: Enable/disable screen");
                                printf("s: Step\n");
                                printf("c: Continue\n");
                                printf("r: Reset\n");
                                printf("m: Memory dump\n");
                                printf("i: Debugger\n");
                                printf("v: See last 10 PC values\n");
                                printf("q: Quit\n");
                                break;
                            case 'g':
                                ui_mutex.lock();
                                ui->change_requested = true;
                                ui_mutex.unlock();
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
                                emu_reset(sem);
                                dbg.debug_print();
                                
                                break;
                            case 'm':
                                memory->dump();
                                break;
                            case 'v':
                                std::cout<<"[";
                                for (int i = 0; i < 10; i++){
                                    std::cout<<numToHexString(dbg.last_pc_values[i], 4)<<" ";
                                }
                                std::cout<<"]"<<std::endl;
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
        //dbg.start_chrono();
        cpu->step();
        controller->process_events();
        //elapsed = dbg.get_chrono();

    }
    //fclose(log_pc);
}
int emu_run(int argc, char** argv){
    std::signal(SIGINT, signal_handler);
    ui->set_debugger(&dbg);
    memory->set_dma(dma);
    memory->set_controller(controller);
    ui->init();
    if(argc < 2 || !memory->load_rom(argv[1])){
        printf("Error loading ROM\n");
        return 1;
    }
    printf("ROM loaded: %s\n\n", memory->rom_header->title);
    cpu->adjust_flag_from_checksum();
    #ifdef TRACEGEN
    dbg.generate_trace_header();
    #endif
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    std::binary_semaphore sem = std::binary_semaphore(0);
    Cpu_thread_args thread_args = {&sem};
    pthread_t cpu_thread;
    pthread_create(&cpu_thread, NULL, (void*(*)(void*))cpu_run, &thread_args);
    while(cpu->state != QUIT){
        delay(10);
        ui_mutex.lock();
        ui->update();
        ui_mutex.unlock();
        if (resetting){
            resetting = false;
            sem.release();
        }
    }
    return 0;
}
void run_ticks(int ticks_to_run){
    for(int m = 0; m < ticks_to_run; m++){
        for (int tick = 0; tick < 4; tick++){
            ticks++;
            timer->tick();
            ppu->tick();
        }
        dma->tick();
    }
}



