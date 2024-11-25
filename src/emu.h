#pragma once
typedef enum {NO_DBG, PRINT_DBG, FULL_DBG} Debug_mode;

int emu_run(int argc, char **argv);