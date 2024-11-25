#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#ifdef _WIN32
    #include <windows.h>
    void createProcess();
#endif
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define GET_BIT(n, bit) ((n >> bit) & 1)
#define SET_BIT(n, bit, value) (n = (n & ~(1 << bit)) | (value << bit))
#define IS_IN_RANGE(n, min, max) (n >= min && n <= max)
#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)
void delay(u32 ms);