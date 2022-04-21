#pragma once

#include <cstdint>

typedef struct DisplaySettings {
    uint16_t lcdWidth   = 480;
    uint16_t lcdHeight  = 272;
    uint16_t lcdHcycle  = 548;
    uint16_t lcdHoffset = 43;
    uint16_t lcdHsync0  = 0;
    uint16_t lcdHsync1  = 41;
    uint16_t lcdVcycle  = 292;
    uint16_t lcdVoffset = 12;
    uint16_t lcdVsync0  = 0;
    uint16_t lcdVsync1  = 10;
    uint16_t lcdPclk    = 5;
    uint8_t lcdSwizzle = 0;
    uint8_t lcdPclkpol = 1;
    uint8_t lcdCspread = 1;
}DisplaySettings_t;
