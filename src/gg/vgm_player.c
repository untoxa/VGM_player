#include <gbdk/platform.h>

#include <stdint.h>
#include <stdbool.h>

#include "vgm_player.h"
#include "pff.h"
#include "joy.h"

static SFR AT(0xf0) OPLL_REG;
static SFR AT(0xf1) OPLL_DATA;
static SFR AT(0xf2) AUDIO_CONTROL;

extern FATFS filesystem;

uint8_t last_vgm_command;

uint32_t vgm_data_size, vgm_read_position, vgm_play_position;

#define READ_BUFFER_SIZE 0x800
#define READ_BUFFER_MASK 0x7ff

uint8_t readahead_buffer[READ_BUFFER_SIZE];
volatile uint8_t isr_skip;
volatile uint8_t isr_result;

void vgm_play_cut(void) {
    // cut PSG
    PSG = PSG_LATCH | PSG_CH0 | PSG_VOLUME | 0x0f;
    PSG = PSG_LATCH | PSG_CH1 | PSG_VOLUME | 0x0f;
    PSG = PSG_LATCH | PSG_CH2 | PSG_VOLUME | 0x0f;
    PSG = PSG_LATCH | PSG_CH3 | PSG_VOLUME | 0x0f;
    // cut FM
    for (uint8_t i = 9; i != 0; --i) {
        OPLL_REG = 0x20 + i;
        OPLL_DATA = 0;
        __asm
            .rept 3
                push hl
                pop hl
            .endm
        __endasm;
    }
}

void vgm_play(void) {
    static uint16_t delay;
    static uint16_t start, stop;

    if (isr_result) return;
    if (isr_skip) {
        --isr_skip;
        return;
    }
    if (vgm_play_position >= vgm_data_size) {
        isr_result = VGM_EOF;
        return;
    }
    start = ((uint16_t)vgm_play_position) & READ_BUFFER_MASK;
    stop = ((uint16_t)vgm_read_position) & READ_BUFFER_MASK;
    while (start != stop) {
        switch (*(readahead_buffer + start)) {
            case 0x50:
                start = ++start & READ_BUFFER_MASK;
                if (start == stop) return;
                PSG = *(readahead_buffer + start);
                start = ++start & READ_BUFFER_MASK;
                vgm_play_position += 2;
                break;
            case 0x51:
                start = ++start & READ_BUFFER_MASK;
                if (start == stop) return;
                OPLL_REG = *(readahead_buffer + start);
                start = ++start & READ_BUFFER_MASK;
                if (start == stop) return;
                OPLL_DATA = *(readahead_buffer + start);
                start = ++start & READ_BUFFER_MASK;
                vgm_play_position += 3;
                break;
            case 0x61:
                start = ++start & READ_BUFFER_MASK;
                if (start == stop) return;
                delay = *((uint16_t *)(readahead_buffer + start));
                start = ++start & READ_BUFFER_MASK;
                if (start == stop) return;
                isr_skip = delay / 735;
                vgm_play_position += 3;
                return;
            case 0x62:
            case 0x63:
                ++vgm_play_position;
                return;
            case 0x66:
                isr_result = VGM_EOF;
                return;
            case 0x4f :
                start = ++start & READ_BUFFER_MASK;
                if (start == stop) return;
                start = ++start & READ_BUFFER_MASK;
                vgm_play_position += 2;
                break;
            default:
                if ((*(readahead_buffer + start) > 0x6f) && (*(readahead_buffer + start) < 0x80)) {
                    start = ++start & READ_BUFFER_MASK;
                    ++vgm_play_position;
                    break;
                }
                last_vgm_command = *(readahead_buffer + start);
                isr_result = VGM_UNSUPORTED_CMD;
                return;
        }
    }
}

#define NTSC_HBLANK_PERIOD 48
#define NTSC_CORRECTION_PERIOD 5
#define NTSC_60HZ_COUNTER 5
#define NTSC_50HZ_COUNTER 6

#define PAL_HBLANK_PERIOD 39
#define PAL_CORRECTION_PERIOD 1
#define PAL_60HZ_COUNTER 5
#define PAL_50HZ_COUNTER 6

bool vblank_triggered = false;
uint8_t interrupt_counter = 0, interrupt_max_counter = NTSC_50HZ_COUNTER, scanline_counter = NTSC_HBLANK_PERIOD, scanline_correction = NTSC_CORRECTION_PERIOD;
void vblank_isr(void) {
    __WRITE_VDP_REG(VDP_R10, scanline_correction);
    vblank_triggered = true;
    if (++interrupt_counter == interrupt_max_counter) {
        vgm_play();
        interrupt_counter = 0;
    }
}
void scanline_isr(void) {
    if (vblank_triggered) {
        __WRITE_VDP_REG(VDP_R10, scanline_counter); // set scanline counter to the target value
        VCOUNTER = 0xff;                            // retrigger the scanline counter
        vblank_triggered = false;
    } else {
        if (++interrupt_counter == interrupt_max_counter) {
            vgm_play();
            interrupt_counter = 0;
        }
    }
}

VGM_RESULT vgm_play_file(const uint8_t * name) {
    static uint32_t temp[5];
    static uint32_t rate;
    static uint32_t data_offset;
    static uint16_t bytes_loaded;
    static uint16_t load;
    static uint32_t ppos;

    AUDIO_CONTROL = AUDIO_CONTROL | 0x03;

    last_vgm_command = 0;

    // open file
    if (pf_open(name) != FR_OK) return VGM_READ_ERROR;

    // read first 4 dwords: TAG, EOF, VERSION, SN Clock
    if (pf_read(temp, sizeof(temp), &bytes_loaded) != FR_OK) return VGM_READ_ERROR;
    if (bytes_loaded != sizeof(temp)) return VGM_FORMAT_ERROR;

    // check format TAG
    if (temp[0] != VGM_MAGIC) return VGM_FORMAT_ERROR;

    // check PSG || FM
    if ((temp[3] == 0) && (temp[4] == 0)) return VGM_UNSUPORTED_CHIP;

    pf_lseek(0x24);
    if (pf_read(&rate, sizeof(rate), &bytes_loaded) != FR_OK) return VGM_READ_ERROR;

    // locate sound data block offset
    if (temp[2] >= VGM_VERSION_1_50) {
        pf_lseek(0x34);
        if (pf_read(&data_offset, sizeof(data_offset), &bytes_loaded) != FR_OK) return VGM_READ_ERROR;
        if (bytes_loaded != sizeof(data_offset)) return VGM_FORMAT_ERROR;
        data_offset += 0x34;
    } else data_offset = 0x40;

    // jump to data block
    if (pf_lseek(data_offset) != FR_OK) return VGM_READ_ERROR;

    vgm_data_size = filesystem.fsize - data_offset;
    vgm_read_position = vgm_play_position = 0;
    isr_result = VGM_OK;
    isr_skip = 0;

    if (get_system() == SYSTEM_60HZ) {
        scanline_counter = NTSC_HBLANK_PERIOD;
        scanline_correction = NTSC_CORRECTION_PERIOD;
        interrupt_max_counter = ((uint8_t)rate == 50) ? NTSC_50HZ_COUNTER : NTSC_60HZ_COUNTER;
    } else {
        scanline_counter = PAL_HBLANK_PERIOD;
        scanline_correction = PAL_CORRECTION_PERIOD;
        interrupt_max_counter = ((uint8_t)rate == 50) ? PAL_50HZ_COUNTER : PAL_60HZ_COUNTER;
    }

    // set the correct settings for the PAL/NTSC compensation
    __WRITE_VDP_REG(VDP_R10, 0x00);

    disable_interrupts();
    remove_VBL(vblank_isr);
    add_VBL(vblank_isr);
    remove_LCD(scanline_isr);
    add_LCD(scanline_isr);
    enable_interrupts();

    set_interrupts(VBL_IFLAG | LCD_IFLAG);

    while (true) {
        PROCESS_INPUT();
        if (KEY_PRESSED(J_A | J_B)) {
            waitpadup();
            break;
        }
        if (isr_result) break;

        disable_interrupts();
        ppos = vgm_play_position;
        enable_interrupts();

        if (vgm_read_position < vgm_data_size) {
            if ((vgm_read_position - ppos) < (READ_BUFFER_SIZE >> 2)) {
                while ((vgm_read_position - ppos) < (READ_BUFFER_SIZE >> 1)) {
                    load = ((uint16_t)vgm_read_position) & READ_BUFFER_MASK;
                    if (pf_read(readahead_buffer + load, 64, &bytes_loaded) != FR_OK) {
                        isr_result = VGM_READ_ERROR;
                        break;
                    }
                    if (!bytes_loaded) break;

                    disable_interrupts();
                    vgm_read_position += bytes_loaded;
                    ppos = vgm_play_position;
                    enable_interrupts();

                    if (vgm_read_position >= vgm_data_size) break;
                }
            }
        }
        vsync();
    }

    disable_interrupts();
    remove_VBL(vblank_isr);
    remove_LCD(scanline_isr);
    enable_interrupts();

    set_interrupts(VBL_IFLAG);

    vgm_play_cut();
    return (isr_result == VGM_EOF) ? VGM_OK : isr_result;
}
