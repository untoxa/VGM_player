#ifndef __SCREEN_H_INCLIDE__
#define __SCREEN_H_INCLIDE__

#include <gbdk/platform.h>
#include <stdint.h>
#include <stdbool.h>

#include "vwf.h"
#include "misc_assets.h"

#if defined(NINTENDO)
#define TILE_BANK_0 _VRAM8800
#define TILE_BANK_1 _VRAM8000
#elif defined(SEGA)
#if defined(MASTERSYSTEM)
static uint8_t AT(0x4000) TILE_BANK_0[];
static uint8_t AT(0x4800) TILE_BANK_1[];
static uint8_t AT(0x5000) TILE_BANK_2[];
#elif defined(GAMEGEAR)
static uint8_t AT(0x4000) TILE_BANK_0[];
static uint8_t AT(0x6000) TILE_BANK_1[];
#endif
#endif

#define TO_TILE_ADDRESS(BASE, NO) ((BASE) + ((NO) << DEVICE_TILE_SIZE_BITS))

BANKREF_EXTERN(module_screen)

extern const uint8_t * const screen_tile_addresses[DEVICE_SCREEN_HEIGHT];
extern const uint8_t screen_tile_map[];

#if defined(MASTERSYSTEM)
extern const uint8_t color_table[];
#endif

inline void screen_set_tile_xy(uint8_t x, uint8_t y, uint8_t tile) {
#if defined(GAMEGEAR)
    set_attributed_tile_xy(x, y, tile);
#else
    set_bkg_tile_xy(x, y, tile);
#endif
}

inline uint8_t screen_clear_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
#if defined(MASTERSYSTEM)
    return (w) ? fill_rect(x, y, w, h, SLD_WHITE + color_table[BG_COLOR(color)]), w : w;
#else
    return (w) ? fill_rect(x, y, w, h, SLD_WHITE + BG_COLOR(color)), w : w;
#endif
}

inline uint8_t screen_restore_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
#if defined(NINTENDO)
    return (w) ? set_bkg_submap(x, y, w, h, screen_tile_map, DEVICE_SCREEN_WIDTH), w : w;
#elif defined(SEGA)
    #if defined(MASTERSYSTEM)
    return (w) ? set_bkg_submap(x, y, w, h, screen_tile_map, DEVICE_SCREEN_WIDTH), w : w;
    #elif defined(GAMEGEAR)
    return (w) ? set_tile_submap(x, y, w, h, DEVICE_SCREEN_WIDTH, screen_tile_map), w : w;
    #endif
#endif
}

inline uint8_t screen_text_render(uint8_t x, uint8_t y, const uint8_t * text) {
    return vwf_draw_text(screen_tile_addresses[y] + (x << DEVICE_TILE_SIZE_BITS), text, 0);
}

inline uint8_t screen_text_out(uint8_t x, uint8_t y, const uint8_t * text) {
    return screen_restore_rect(x, y, vwf_draw_text(screen_tile_addresses[y] + (x << DEVICE_TILE_SIZE_BITS), text, 0), 1);
}

uint8_t INIT_module_screen(void) BANKED;

#endif