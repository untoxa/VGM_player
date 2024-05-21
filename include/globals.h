#ifndef __GLOBALS_H_INCLUDE__
#define __GLOBALS_H_INCLUDE__

#include <stdint.h>

#define MAX_TEXT_BUFFER_SIZE 256

// width of the help context display in tiles
#define HELP_CONTEXT_WIDTH 17

// enable debug menu item
#ifndef DEBUG_ENABLED
    #define DEBUG_ENABLED 0
#endif

extern uint8_t text_buffer[MAX_TEXT_BUFFER_SIZE];   // temporary buffer for rendering of text
#define text_buffer_extra (text_buffer + (MAX_TEXT_BUFFER_SIZE / 3))
#define text_buffer_extra_ex (text_buffer + ((MAX_TEXT_BUFFER_SIZE / 3) * 2))

#endif
