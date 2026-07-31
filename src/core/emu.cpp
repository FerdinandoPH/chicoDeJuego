#include "emu.h"
#include "prefs.h"
#include "debugger.h"
#include "memory.h"
#include "utils.h"
#include "cpu.h"
#include "timer.h"
#include <stdio.h>
#include "host.h"
#include "ppu.h"
#include "dma.h"
#include "apu.h"
#include "controller.h"
#include "sync.h"
#include "savestates.h"
#include "hw_reg_def.h"
#include <thread>
#include <iostream>
#include <csignal>
#include <mutex>
#include <chrono>

Prefs prefs = Prefs{.force_dmg = false};
Debug_mode initial_dbg_mode = NO_DBG;
GB_model gb_model = GB_model::DMG;
Memory* memory = new Memory(gb_model, &prefs);
Controller* controller = new Controller(*memory);

Dma* dma = new Dma(memory);


int ticks_per_frame = 70224;
Cpu* cpu = new Cpu(*memory, gb_model, ticks_per_frame);
Timer* timer = new Timer(*cpu, *memory);
int ticks = 0;
int ticks_since_last_sync = 0;

bool resetting = false;
Host* host = new Host(*memory, *controller, 4);
Apu* apu = new Apu(*memory, *cpu, *host);
Ppu* ppu = new Ppu(*memory, host, gb_model);

Vdma* vdma = new Vdma(memory, ppu, cpu);
std::mutex host_mutex = std::mutex();
Debugger dbg = Debugger(initial_dbg_mode, ticks, *memory, *cpu, *timer, *ppu);
SaveStateManager* ssm = new SaveStateManager(cpu, timer, ppu, memory, dma, vdma, host, apu, ticks, ticks_since_last_sync);
Emu_sync* sync_controller = new Emu_sync(ticks, ticks_since_last_sync, memory, controller, host);
void signal_handler(int signal){
    if (signal == SIGINT){
        std::signal(SIGINT, signal_handler);
        if (dbg.dbg_level != FULL_DBG){
            std::cout << "SIGINT detected"<<std::endl<<std::endl;
            dbg.dbg_level = FULL_DBG;
        }
        else{
            cpu->set_state(QUIT);
            std::exit(1);
        }
    }
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
void debug_menu(std::binary_semaphore* sem){
    memory->sync_mem_ui_copy();
    host->sync_video_buffer();
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
                printf("x: Delete breakpoint\n");
                printf("g: Toggle debug windows\n");
                printf("p: Clear main screen\n");
                printf("s: Step\n");
                printf("c: Continue\n");
                printf("r: Reset\n");
                printf("m: Memory dump\n");
                printf("i: Debugger\n");
                printf("v: See last 10 PC values\n");
                printf("q: Quit\n");
                break;
            case 'g': {
                printf("Toggle debug windows:\n");
                printf("  1. Tile Viewer  [%s]\n", host->is_debug_window_active(DebugWindowType::TILES)   ? "ON" : "OFF");
                printf("  2. BG Map       [%s]\n", host->is_debug_window_active(DebugWindowType::BG_MAP)  ? "ON" : "OFF");
                printf("  3. Window Map   [%s]\n", host->is_debug_window_active(DebugWindowType::WIN_MAP) ? "ON" : "OFF");
                printf("  4. OAM Sprites  [%s]\n", host->is_debug_window_active(DebugWindowType::OAM)     ? "ON" : "OFF");
                printf("  0. Cancel\n");
                std::string choice;
                fflush(stdin);
                std::getline(std::cin, choice);
                int idx = choice[0] - '1';
                if (idx >= 0 && idx < NUM_DEBUG_WINDOWS){
                    host->debug_toggle_requested[idx] = true;
                }
                break;
            }
            case 'p':
                host->clear_main_screen();
                break;
            case 'q':
                printf("Quitting...\n");
                exit = true;
                cpu->set_state(QUIT);
                break;
            case 'c':
                dbg.dbg_level = OFF_DBG;
                exit = true;
                break;
            case 'r':
                printf("Resetting is dangerous rn...\n");
                // emu_reset(sem);
                // dbg.debug_print();
                
                break;
            case 'm':
                if (memory->dump())
                    host->open_with_default_app("mem.hexd"); // opens it with whatever the system uses for .hexd
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
void* cpu_run(void* thread_args){
    std::binary_semaphore* sem = ((Cpu_thread_args*)thread_args)->sem;
    //std::chrono::duration<double, std::micro> elapsed = dbg.get_chrono();
    //FILE* log_pc = fopen("chicoDeJuego.emulog", "wb");
    while(cpu->get_state() != QUIT){
        if(cpu->check_interrupts() && (dbg.dbg_level == FULL_DBG || dbg.dbg_level == PRINT_DBG)){
            printf("%s interrupt triggered\n",interrupt_names.at(cpu->regs[PC]).c_str());
        }
        if(cpu->get_state() == PAUSED){
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
                    host->clear_speed_percent();
                    debug_menu(sem);
                    sync_controller->reset_speed_window();
                }
            }
        }
        //dbg.start_chrono();
        cpu->step();
        if(ticks_since_last_sync >=ticks_per_frame && (ppu->vblank_triggered || !ppu->is_enabled())){ //Sync on each VBlank
            if (ppu->vblank_triggered)
                ppu->vblank_triggered = false;
            sync_controller->sync();
        }
        //elapsed = dbg.get_chrono();

    }
    //fclose(log_pc);
    return nullptr;
}

int emu_run(int argc, char** argv){
    std::signal(SIGINT, signal_handler);
    host->set_debugger(&dbg);
    memory->set_dma(dma);
    memory->set_controller(controller);
    memory->set_apu(apu);
    memory->set_vdma(vdma);
    cpu->set_timer(timer);
    cpu->set_apu(apu);
    host->init();
    controller->set_sync_controller(sync_controller);
    controller->set_save_state_manager(ssm);
    if(argc < 2 || !memory->load_rom(argv[1])){
        printf("Error loading ROM\n");
        return 1;
    }
    printf("ROM loaded: %s\n\n", memory->rom_header.title);
    cpu->reset(); // load GBC if applicable
    ssm->set_filename(std::string(memory->rom_header.title) + ".state");
    cpu->adjust_flag_from_checksum();
    #ifdef TRACEGEN
    dbg.generate_trace_header();
    #endif
    std::binary_semaphore sem = std::binary_semaphore(0);
    Cpu_thread_args thread_args = {&sem};
    std::thread cpu_thread(cpu_run, &thread_args);
    //host->debug_toggle_requested[1] = true; //for debug only
    while(cpu->get_state() != QUIT){
        host_mutex.lock();
        if(!host->update())
            cpu->set_state(QUIT);
        host_mutex.unlock();
        if (resetting){
            resetting = false;
            sem.release();
        }
    }
    cpu_thread.join();
    memory->close();
    return 0;
}
void run_ticks(int ticks_to_run){
    // for(int m = 0; m < ticks_to_run; m++){
    //     for (int tick = 0; tick < 4; tick++){
    //         ticks++;
    //         ticks_since_last_sync++;
    //         timer->tick();
    //         ppu->tick();
    //         apu->tick();
    //     }
    //     dma->tick();
    // }
    int tick_goal = ticks + ticks_to_run;
    for(int i = ticks; i < tick_goal; i++){

        
        for (int j = 0; j < 4; j++){
            //Always 4
            ticks++;
            ticks_since_last_sync++;
            timer->tick();
            // 2 or 4 depending on speed
            if(j % (cpu->get_speed_mode() == Speed_mode::NORMAL ? 1 : 2) == 0){
                ppu->tick();
                apu->tick();
            }
        }



        //Always 1
        dma->tick();

        //2 or 1 depending on speed
        if(gb_model == GB_model::CGB){
            for (int j = 0; j < (cpu->get_speed_mode() == Speed_mode::NORMAL ? 2 : 1); j++){
                vdma->tick();
            }
        }
    }
}



