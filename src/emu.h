#pragma once

#include <utils.h>
#include <mem.h>
typedef enum{RUN, PAUSE, STOP, HALT, QUIT} State;
typedef enum {NONE, PRINT, FULL} DebugMode;
typedef struct{
    State state;
    u64 ticks;
} EmuData;

int emuRun(int argc, char **argv);
static Memory mem;
EmuData *emuGetData();
void cycle(int cycles);