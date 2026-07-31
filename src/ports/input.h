#pragma once

// Host keys the emulator understands. Deliberately only the ones actually used:
// translating from the native key code (SDL_Scancode and friends) is the
// platform backend's job, not the core's.
enum class Host_key {
    UP, DOWN, LEFT, RIGHT,      // d-pad
    Z, X, RETURN, BACKSPACE,    // A, B, START, SELECT
    SPACE, F1, F2,              // turbo, save state, load state
    ESCAPE,                     // open the debugger menu
    UNKNOWN
};

enum class Controller_event_type{KEY_DOWN, KEY_UP};

typedef struct{
    Controller_event_type type;
    Host_key key;
} Input_event;
