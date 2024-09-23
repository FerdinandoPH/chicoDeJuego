#pragma once

#include <utils.h>
typedef enum{RUN, PAUSE, STOP, HALT, QUIT} State;
typedef struct{
    State state;
    u64 ticks;
} emu_data;

int emu_run(int argc, char **argv);

emu_data *emu_get_data();