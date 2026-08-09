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
#include "run_control.h"
#include "debug_api.h"
#include "core_log.h"
#include "menu.h"
#include <thread>
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
Run_control run_control = Run_control();
Debug_api* debug_api = new Debug_api(dbg, *cpu, *memory, *host);
SaveStateManager* ssm = new SaveStateManager(cpu, timer, ppu, memory, dma, vdma, host, apu, ticks, ticks_since_last_sync);
Emu_sync* sync_controller = new Emu_sync(ticks, ticks_since_last_sync, memory, controller, host);
void signal_handler(int signal){
    if (signal == SIGINT){
        std::signal(SIGINT, signal_handler);
        if (dbg.dbg_level != FULL_DBG){
            log_info("SIGINT detected\n\n");
            run_control.request_break();
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
// Hands the machine over to the backend's menu and waits there. Called from the
// emulation thread and only at an instruction boundary, so everything the menu
// can look at is consistent; emulation does not advance until it returns.
void enter_debug_menu(){
    memory->sync_mem_ui_copy();
    host->sync_video_buffer();
    run_control.enter_paused();
    platform_debug_menu(*debug_api);
    run_control.leave_paused();
    log_info("\n");
}
void* cpu_run(void* thread_args){
    // The semaphore in thread_args was only there for the menu to hand the reset
    // over to the main thread. The menu is gone from here, and the reset it
    // guarded is still commented out, so nothing reads it for now.
    (void)thread_args;
    //std::chrono::duration<double, std::micro> elapsed = dbg.get_chrono();
    //FILE* log_pc = fopen("chicoDeJuego.emulog", "wb");
    while(cpu->get_state() != QUIT){
        if(cpu->check_interrupts() && (dbg.dbg_level == FULL_DBG || dbg.dbg_level == PRINT_DBG)){
            log_info("%s interrupt triggered\n", interrupt_names.at(cpu->regs[PC]).c_str());
        }
        if(cpu->get_state() == PAUSED){
            continue;
        }
        #ifdef TRACEGEN
            dbg.generate_trace();
        #endif
        // A break asked for from another thread (ESC, Ctrl-C) is only honored
        // here: this is the boundary where stopping is safe.
        if(run_control.take_break_request()){
            dbg.dbg_level = FULL_DBG;
        }
        if(dbg.dbg_level != NO_DBG){
            dbg.check_breakpoints();
            if(dbg.dbg_level == PRINT_DBG || dbg.dbg_level == FULL_DBG){
                //std::cout<<"Elapsed time: "<<elapsed.count()<<"us"<<std::endl;
                dbg.debug_print();
                if(dbg.dbg_level == FULL_DBG){
                    host->clear_speed_percent();
                    enter_debug_menu();
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
    host->set_run_control(&run_control);
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
        log_error("Error loading ROM\n");
        return 1;
    }
    log_info("ROM loaded: %s\n\n", memory->rom_header.title);
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



