#ifndef __SYSTEMDETECT_H_INCLUDE__
#define __SYSTEMDETECT_H_INCLUDE__

#include <gbdk/platform.h>
#include <stdint.h>
#include <stdbool.h>

extern bool _is_SUPER, _is_COLOR, _is_ADVANCE;
extern bool _is_CPU_FAST;

uint8_t CPU_FAST(void) BANKED;
void CPU_SLOW(void) BANKED;

uint8_t detect_system(void) BANKED;

uint8_t setup_system(void) BANKED;

#endif