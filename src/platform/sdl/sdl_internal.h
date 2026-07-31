#pragma once
#include <SDL3/SDL.h>

// Details shared between the .cpp files of the SDL backend, not visible from the
// core. Sdl_system needs them to tell which window a close event belongs to,
// something only Sdl_video knows.
SDL_WindowID sdl_main_window_id();
SDL_WindowID sdl_aux_window_id(int i);
