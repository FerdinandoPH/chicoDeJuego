#pragma once

#include <utils.h>
#include <mem.h>

typedef struct{
    u8 entry[4];
    u8 nintendoLogo[0x30];
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
} CartHeader;
bool cartLoad(const char *filename, Memory *mem, u16 from, u16 to);