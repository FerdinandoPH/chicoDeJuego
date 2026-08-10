#pragma once
extern int ticks;

int emu_run(int argc, char **argv);
void run_ticks(int ticks_to_run);
// Back to the state right after booting, keeping the cartridge's RAM. Only safe
// from the emulation thread (the debug menu runs there).
void emu_reset();