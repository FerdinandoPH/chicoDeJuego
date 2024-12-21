#pragma once
#include <cstdint>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <iomanip>
#include <variant>
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
    void createProcess(const char *proc);
#endif
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define GET_BIT(n, bit) ((n >> bit) & 1)
#define SET_BIT(n, bit, value) (n = (n & ~(1 << bit)) | (value << bit))
#define BETWEEN(n, min, max) (n >= min && n <= max)
#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)
void delay(u32 ms);
template <typename T>
std::string numToHexString(T value, int width=0, bool is_imme8 = false){
    std::ostringstream oss;
    if(is_imme8){
        int8_t signed_value = static_cast<int8_t>(value);
        if(signed_value < 0){
            oss << "-";
            value = static_cast<u16> (-signed_value);
        }
    } 
    oss << "0x";
    oss << std::hex << std::uppercase;
    if (width>1)
        oss << std::setw(width) << std::setfill('0') << value;
    else
        oss << std::setw(1) << std::setfill('0') << value;
    return oss.str();
}