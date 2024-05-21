#ifndef _JOY_H_DEFINED
#define _JOY_H_DEFINED

extern uint8_t joy, old_joy;

inline void RESET_INPUT(void) {
    old_joy = joy = 0;
}

inline void JOYPAD_INPUT(void) {
    old_joy = joy, joy = joypad();
}
#define PROCESS_INPUT JOYPAD_INPUT

inline uint8_t KEY_PRESSED(uint8_t key) {
    return ((joy & ~old_joy) & key);
}

#endif