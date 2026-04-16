#pragma once
#include "utils.h"
#define XRES 160
#define YRES 144
const u32 gb_palette[4] = {0xFF9BBC0F, 0xFF8BAC0F, 0xFF306230, 0xFF0F380F};
const u32 GRID_COLOR = 0xFF555555;

// Debug windows

// Tile viewer: 16 cols x 24 rows of 8x8 tiles, 1px gap between tiles
#define TILE_DBG_W (16 * 9)   // 144
#define TILE_DBG_H (24 * 9)   // 216

// BG Map: 32x32 tiles of 8x8, 1px gap
#define BG_MAP_DBG_W (32 * 9) // 288
#define BG_MAP_DBG_H (32 * 9) // 288

// Window Map: same as BG Map
#define WIN_MAP_DBG_W (32 * 9) // 288
#define WIN_MAP_DBG_H (32 * 9) // 288

// OAM Sprites: 10 cols x 4 rows, 8x16, 1px gap
#define OAM_DBG_W (10 * 9)   // 90
#define OAM_DBG_H (4 * 17)   // 68