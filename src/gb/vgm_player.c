#include <gbdk/platform.h>

#include <stdint.h>
#include <stdbool.h>

#include "vgm_player.h"
#include "pff.h"
#include "joy.h"

#define TICKS_PER_FRAME 735

extern FATFS filesystem;

uint8_t last_vgm_command;

uint32_t vgm_data_size, vgm_read_position, vgm_play_position;

#define READ_BUFFER_SIZE 0x800
#define READ_BUFFER_MASK 0x7ff

uint8_t readahead_buffer[READ_BUFFER_SIZE];
volatile uint8_t isr_skip;
volatile uint8_t isr_result;

typedef uint8_t mutex_t;

#define MUTEX_INIT_VALUE 0b11111110

uint8_t mutex_try_lock(mutex_t * mutex) PRESERVES_REGS(b, c) NAKED {
    mutex;
__asm
        ld      h, d
        ld      l, e
        xor     a
        sra     (hl)
        ccf
        rla
        ret
__endasm;
}
void mutex_unlock(mutex_t * mutex) PRESERVES_REGS(b, c) NAKED {
    mutex;
__asm
        ld      h, d
        ld      l, e
        res     0, (hl)
        ret
__endasm;
}

inline void vgm_play_cut(void) {
    NR52_REG = 0;
}

void vgm_play(void) {
    static uint16_t delay;
    static uint8_t addr, data;
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
            case 0xB3: /* write value to register */
                start = ++start & READ_BUFFER_MASK;
                if (start == stop) return;
                addr = *(readahead_buffer + start);
                start = ++start & READ_BUFFER_MASK;
                if (start == stop) return;
                data = *(readahead_buffer + start);
                if (addr < 0x30) {
                    *((uint8_t *)0xff10 + addr) = data;
                }
                start = ++start & READ_BUFFER_MASK;
                vgm_play_position += 3;
                break;
            case 0x61:
                start = ++start & READ_BUFFER_MASK;
                if (start == stop) return;
                delay = *((uint16_t *)(readahead_buffer + start));
                start = ++start & READ_BUFFER_MASK;
                if (start == stop) return;
                isr_skip = delay / TICKS_PER_FRAME;
                vgm_play_position += 3;
                return;
            case 0x62:
            case 0x63:
                ++vgm_play_position;
                return;
            case 0x66:
                isr_result = VGM_EOF;
                return;
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

volatile mutex_t mutex;

void timer_isr(void) {
    if (mutex_try_lock(&mutex)) {
        vgm_play();
        mutex_unlock(&mutex);
    }
}

VGM_RESULT vgm_play_file(const uint8_t * name) {
    static uint32_t temp[3];
    static uint32_t data_offset;
    static uint16_t bytes_loaded;
    static uint16_t load;
    static uint32_t ppos;

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
    vgm_data_size = filesystem.fsize - data_offset;
    vgm_read_position = vgm_play_position = 0;
    isr_result = VGM_OK;
    isr_skip = 0;

    // init mutex
    mutex = MUTEX_INIT_VALUE;

    // init timer interrupt
    TMA_REG = (_cpu == CGB_TYPE) ? 0x78u : 0xBCu, TAC_REG = 0x04u;
    CRITICAL {
        remove_TIM(timer_isr);
        add_low_priority_TIM(timer_isr);
    }
    set_interrupts(VBL_IFLAG | LCD_IFLAG | TIM_IFLAG);

    while (true) {
        PROCESS_INPUT();
        if (KEY_PRESSED(J_A | J_B)) {
            waitpadup();
            break;
        }
        if (isr_result) break;

        CRITICAL {
            ppos = vgm_play_position;
        }

        if (vgm_read_position < vgm_data_size) {
            if ((vgm_read_position - ppos) < (READ_BUFFER_SIZE >> 2)) {
                while ((vgm_read_position - ppos) < (READ_BUFFER_SIZE >> 1)) {
                    load = ((uint16_t)vgm_read_position) & READ_BUFFER_MASK;
                    if (pf_read(readahead_buffer + load, 64, &bytes_loaded) != FR_OK) {
                        isr_result = VGM_READ_ERROR;
                        break;
                    }
                    if (!bytes_loaded) break;

                    CRITICAL {
                        vgm_read_position += bytes_loaded;
                        ppos = vgm_play_position;
                    }

                    if (vgm_read_position >= vgm_data_size) break;
                }
            }
        }
        vsync();
    }

    set_interrupts(VBL_IFLAG | LCD_IFLAG);
    CRITICAL {
        remove_TIM(timer_isr);
    }

    vgm_play_cut();
    return (isr_result == VGM_EOF) ? VGM_OK : isr_result;
}
