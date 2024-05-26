#pragma bank 255

#include <gbdk/platform.h>
#include <stdint.h>
#include <stdbool.h>

#include "systemdetect.h"

bool _is_SUPER, _is_COLOR, _is_ADVANCE;
bool _is_CPU_FAST;

uint8_t detect_system(void) BANKED {
#if defined(NINTENDO)
    // For the SGB + PAL SNES setup this delay is required on startup, otherwise borders don't show up
    for (uint8_t i = 0; i != 4; i++) vsync();

    _is_SUPER    = sgb_check();
    _is_COLOR    = ((!_is_SUPER) && (_cpu == CGB_TYPE) && (*(uint8_t *)0x0143 & 0x80));
    _is_ADVANCE  = (_is_GBA && _is_COLOR);
    _is_CPU_FAST = false;
#elif defined(SEGA)
    _is_SUPER    = false;
    #if defined(MASTERSYSTEM)
    _is_COLOR    = false;
    #elif defined(GAMEGEAR)
    _is_COLOR    = true;
    #endif
    _is_ADVANCE  = false;
    _is_CPU_FAST = false;
#endif
    return 0;
}

uint8_t setup_system(void) BANKED {
#if defined(NINTENDO)
    if (_is_COLOR) {
        CPU_FAST();
        cgb_compatibility();
    }
#elif defined(SEGA)
    #if defined(MASTERSYSTEM)
    // set graphics mode 2
    __WRITE_VDP_REG(VDP_R0, ((__READ_VDP_REG(VDP_R0) & ~0b00000110) | 0b00000010)); // mode 2
    __WRITE_VDP_REG(VDP_R1, (__READ_VDP_REG(VDP_R1) & ~0b00010000));                // mode 2
    __WRITE_VDP_REG(VDP_R2, 0x07); // name table from 0x1C00
    __WRITE_VDP_REG(VDP_R3, 0xff); // color data from 0x2000
    __WRITE_VDP_REG(VDP_R4, 0x03); // tile tada from 0x0000
    __WRITE_VDP_REG(VDP_R5, 0x36); // SAT from 1B00
    #elif defined(GAMEGEAR)
    __WRITE_VDP_REG(VDP_R2, R2_MAP_0x3800);
    __WRITE_VDP_REG(VDP_R5, R5_SAT_0x3F00);
    #endif
#endif
    HIDE_SPRITES; SHOW_BKG;
    DISPLAY_ON;
    return 0;
}

uint8_t CPU_FAST(void) BANKED {
#if defined(NINTENDO)
    return (_is_CPU_FAST = (_is_COLOR) ? (cpu_fast(), true) : false);
#elif defined(SEGA)
    return false;
#endif
}

void CPU_SLOW(void) BANKED {
#if defined(NINTENDO)
    if (_is_COLOR) _is_CPU_FAST = (cpu_slow(), false);
#endif
}
