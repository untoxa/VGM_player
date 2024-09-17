#ifndef _VGM_PLAYER_H_DEFINED
#define _VGM_PLAYER_H_DEFINED

#include <gbdk/platform.h>

#include <stdint.h>

#define VGM_MAGIC 0x206D6756

#define VGM_VERSION_1_00 0x100
#define VGM_VERSION_1_01 0x101
#define VGM_VERSION_1_10 0x110
#define VGM_VERSION_1_50 0x150
#define VGM_VERSION_1_51 0x151
#define VGM_VERSION_1_60 0x160
#define VGM_VERSION_1_61 0x161
#define VGM_VERSION_1_70 0x170
#define VGM_VERSION_1_71 0x171
#define VGM_VERSION_1_72 0x172

typedef enum {
    VGM_OK = 0,
    VGM_READ_ERROR,
    VGM_FORMAT_ERROR,
    VGM_UNSUPORTED_CHIP,
    VGM_VERSION_ERROR,
    VGM_UNSUPORTED_CMD,
    VGM_EOF,
    N_VGM_RESULTS
} VGM_RESULT;

extern uint8_t last_vgm_command;

VGM_RESULT vgm_play_file(const uint8_t * name);

#endif
