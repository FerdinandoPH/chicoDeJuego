#pragma once
#include "utils.h"

// Auxiliary debug windows. The core decides which ones exist and what they
// contain; the backend only knows how to open, close and blit pixels to them.
enum class DebugWindowType { TILES = 0, BG_MAP = 1, WIN_MAP = 2, OAM = 3 };
#define NUM_DEBUG_WINDOWS 4

// Video output. Every call happens once per frame (~60/s), never per pixel: the
// core fills the framebuffer and only the finished result is presented here.
struct IVideo {
    virtual ~IVideo() = default;

    // The core passes the dimensions so the backend doesn't depend on the Game
    // Boy constants.
    virtual bool init(int width, int height, int scale) = 0;
    virtual void shutdown() = 0;

    // Main screen. fb points to width*height pixels in ARGB8888.
    virtual void present(const u32* fb) = 0;
    virtual void set_title(const char* title) = 0;

    // Debug windows. The width is passed on every present_aux because the tile
    // viewer changes size in CGB mode (a decision that belongs to the core).
    virtual void open_aux(DebugWindowType w, int width, int height,
                          int scale, const char* title) = 0;
    virtual void close_aux(DebugWindowType w) = 0;
    virtual void present_aux(DebugWindowType w, const u32* fb, int width) = 0;
};
