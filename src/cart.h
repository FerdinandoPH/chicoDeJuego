#pragma once

#include <utils.h>
#include <mem.h>

typedef struct{
    u8 entry[4];

    char title[16];
    u16 licenseCode;
    u16 licenseCodeNew;
    u8 sgbFlag;
    u8 cartType;
    u8 romSize;
    u8 ramSize;
    u8 regionCode;
    u8 version;
    u8 headerChecksum;
    u16 globalChecksum;
} cartHeader;
bool cart_load(const char *filename, Memory *mem, u16 from, u16 to);