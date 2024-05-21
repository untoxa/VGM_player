#pragma bank 255

#include <gbdk/platform.h>
#include <stdint.h>

#include "vwf.h"
#include "module_vwf.h"

// graphic assets
#include "font_main.h"

#if defined(SEGA)
#define DMG_BLACK     0x03
#define DMG_DARK_GRAY 0x02
#define DMG_LITE_GRAY 0x01
#define DMG_WHITE     0x00
#endif

BANKREF(module_vwf)

// initialize the VWF subsystem
uint8_t INIT_module_vwf(void) BANKED {
    vwf_load_font(0, font_main, BANK(font_main));
    vwf_activate_font(0);
    vwf_set_colors(DMG_BLACK, DMG_WHITE);
    return 0;
}