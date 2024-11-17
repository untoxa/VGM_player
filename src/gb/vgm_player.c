#include <gbdk/platform.h>

#include <stdint.h>
#include <stdbool.h>

#include "vgm_player.h"
#include "pff.h"
#include "joy.h"

#define TICKS_PER_FRAME 735

#define PLAY_BUFFER_SIZE 128

uint8_t last_vgm_command;

uint8_t play_buffer[PLAY_BUFFER_SIZE];
uint8_t *play_load;

#define LOAD_BUFFER_SIZE 64

uint8_t load_buffer[LOAD_BUFFER_SIZE];
uint8_t * load_ptr;
#if LOAD_BUFFER_SIZE > 255
uint16_t bytes_loaded;
#else
uint8_t bytes_loaded;
#endif

inline void read_init(void) {
    load_ptr = load_buffer; bytes_loaded = 0;
}

inline uint8_t read_byte(void) {
    while (true) {
        if (load_ptr < (load_buffer + bytes_loaded)) {
            return *load_ptr++;
        }
        if (pf_read(load_ptr = load_buffer, sizeof(load_buffer), &bytes_loaded) != FR_OK) return 0x66;
    }
}

inline void vgm_play_cut(void) {
    NR52_REG = 0;
}

#if 0
static void vgm_play_buffer(uint8_t count) {
    uint8_t addr;
    uint16_t *ptr = play_buffer;

    for (; count; count -= 2) {
        addr = *ptr++;
        *((volatile uint8_t*) (0xFF00 | addr)) = *ptr++;
    }
}
#else
static void vgm_play_buffer(uint8_t count) PRESERVES_REGS(d, e) NAKED {
    count;
    __asm
        srl a
        ret z
        ld b, a
        ld hl, #_play_buffer
1$:
        ld a, (hl+)
        ld c, a
        ld a, (hl+)
        ldh (c), a
        dec b
        jr nz, 1$

        ret
    __endasm;
}
#endif

VGM_RESULT vgm_play_file(const uint8_t * name) {
    static uint32_t temp[3];
    static uint32_t data_offset;
    static uint16_t delay;
    static uint8_t addr;

    last_vgm_command = 0;

    // open file
    if (pf_open(name) != FR_OK) return VGM_READ_ERROR;

    // read first 3 dwords: TAG, EOF, VERSION
    if (pf_read(temp, sizeof(temp), &bytes_loaded) != FR_OK) return VGM_READ_ERROR;
    if (bytes_loaded != sizeof(temp)) return VGM_FORMAT_ERROR;

    // check format TAG, VERSION
    if (temp[0] != VGM_MAGIC || temp[2] < VGM_VERSION_1_61) return VGM_FORMAT_ERROR;

    // check format clock
    pf_lseek(0x80);
    if (pf_read(temp, sizeof(uint32_t), &bytes_loaded) != FR_OK) return VGM_READ_ERROR;
    if (bytes_loaded != sizeof(uint32_t)) return VGM_FORMAT_ERROR;
    if (temp[0] == 0) return VGM_UNSUPORTED_CHIP;

    // locate sound data block offset
    pf_lseek(0x34);
    if (pf_read(&data_offset, sizeof(data_offset), &bytes_loaded) != FR_OK) return VGM_READ_ERROR;
    if (bytes_loaded != sizeof(data_offset)) return VGM_FORMAT_ERROR;
    data_offset += 0x34;

    // jump to data block
    if (pf_lseek(data_offset) != FR_OK) return VGM_READ_ERROR;

    // init sound
    NR52_REG = 0x80, NR51_REG = 0xFF, NR50_REG = 0x77;

    // play VGM
    play_load = play_buffer;
    read_init();
    while (true) {
        switch (last_vgm_command = read_byte()) {
            case 0xB3: /* write value to register */
                addr = read_byte();
                if (addr < 0x30) {
                    *play_load++ = addr + 0x10;
                    *play_load++ = read_byte();
                } else {
                    read_byte();
                }
                break;
            case 0x61:
                delay = (uint16_t)read_byte() | (((uint16_t)read_byte()) << 8);
                for (int16_t i = (delay / TICKS_PER_FRAME); (i); --i) vsync();
            case 0x62:
            case 0x63:
                vsync();
                vgm_play_buffer(play_load - play_buffer);
                play_load = play_buffer;
                PROCESS_INPUT();
                if (KEY_PRESSED(J_A | J_B)) {
                    vgm_play_cut();
                    waitpadup();
                    return VGM_OK;
                }
                break;
            default:
                // skip unsupported 0x7x waiting commands without breaking
                if ((last_vgm_command > 0x6f) && (last_vgm_command < 0x80)) {
                    vgm_play_buffer(play_load - play_buffer);
                    play_load = play_buffer;
                    break;
                }
                // break on unsupported commands
                vgm_play_cut();
                return VGM_UNSUPORTED_CMD;
            case 0x66:
                vgm_play_cut();
                return VGM_OK;
        }
    }
}
