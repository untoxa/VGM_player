#include <gbdk/platform.h>

#include <stdint.h>
#include <stdbool.h>

#include "vgm_player.h"
#include "pff.h"
#include "joy.h"

#define PLAY_BUFFER_SIZE 32

uint8_t play_buffer[PLAY_BUFFER_SIZE];
uint8_t * play_ptr, *play_load;

#define LOAD_BUFFER_SIZE 64

uint8_t load_buffer[LOAD_BUFFER_SIZE];
uint8_t * load_ptr;
uint16_t bytes_loaded;

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

void vgm_play_cut(void) {
    PSG = PSG_LATCH | PSG_CH0 | PSG_VOLUME | 0x0f;
    PSG = PSG_LATCH | PSG_CH1 | PSG_VOLUME | 0x0f;
    PSG = PSG_LATCH | PSG_CH2 | PSG_VOLUME | 0x0f;
    PSG = PSG_LATCH | PSG_CH3 | PSG_VOLUME | 0x0f;
}

VGM_RESULT vgm_play_file(const uint8_t * name) {
    static uint32_t temp[4];
    static uint32_t data_offset;
    static uint16_t delay;

    // open file
    if (pf_open(name) != FR_OK) return VGM_READ_ERROR;

    // read first 4 dwords: TAG, EOF, VERSION, SN Clock
    if (pf_read(temp, sizeof(temp), &bytes_loaded) != FR_OK) return VGM_READ_ERROR;
    if (bytes_loaded != sizeof(temp)) return VGM_FORMAT_ERROR;

    // check format TAG
    if (temp[0] != VGM_MAGIC) return VGM_FORMAT_ERROR;

    // check PSG
    if (temp[3] == 0) return VGM_UNSUPORTED_CHIP;

    // locate sound data block offset
    if (temp[2] >= VGM_VERSION_1_50) {
        pf_lseek(0x34);
        if (pf_read(&data_offset, sizeof(data_offset), &bytes_loaded) != FR_OK) return VGM_READ_ERROR;
        if (bytes_loaded != sizeof(data_offset)) return VGM_FORMAT_ERROR;
        data_offset += 0x34;
    } else data_offset = 0x40;

    // jump to data block
    if (pf_lseek(data_offset) != FR_OK) return VGM_READ_ERROR;

    play_load = play_buffer;
    read_init();
    while (true) {
        switch (read_byte()) {
            case 0x50 :
                *play_load++ = read_byte();
                break;
            case 0x61 :
                delay = (uint16_t)read_byte() | (((uint16_t)read_byte()) << 8);
                for (uint8_t i = (delay / 735); (i); --i) vsync();
            case 0x62:
            case 0x63:
                vsync();
                for (play_ptr = play_buffer; play_ptr != play_load; PSG = *play_ptr++);
                play_load = play_buffer;
                PROCESS_INPUT();
                if (KEY_PRESSED(J_B)) {
                    vgm_play_cut();
                    waitpadup();
                    return VGM_OK;
                }
                break;
            default:
                vgm_play_cut();
                return VGM_UNSUPORTED_CMD;
            case 0x4f :
                read_byte();
                break;
            case 0x66 :
                vgm_play_cut();
                return VGM_OK;
        }
    }
}
