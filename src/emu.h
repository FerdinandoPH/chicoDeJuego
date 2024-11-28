#pragma once
typedef enum {NO_DBG, PRINT_DBG, FULL_DBG} Debug_mode;
extern int ticks;
int emu_run(int argc, char **argv);
void run_ticks(int ticks_to_run);