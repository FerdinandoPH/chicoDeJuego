#pragma once
#include <semaphore>
extern int ticks;
int emu_run(int argc, char **argv);
void run_ticks(int ticks_to_run);
typedef struct{
    std::binary_semaphore* sem;
}Cpu_thread_args;