#ifndef _EVERDRIVEX_H_DEFINED
#define _EVERDRIVEX_H_DEFINED

// Registers
static volatile uint8_t AT(0xBD00) EDX_REG_SPI_DATA;
static volatile uint8_t AT(0xBD01) EDX_REG_SPI_CTRL;
static volatile uint8_t AT(0xBD0A) EDX_REG_KEY;

// Bit numbers
#define EDX_REG_SPI_SS         0
#define EDX_REG_SPI_LOW_SPEED  1
#define EDX_REG_SPI_AREAD      2
#define EDX_REG_SPI_BUSY       7

// Constants
#define EDX_REG_KEY_UNLOCK 0xA5
#define EDX_REG_KEY_LOCK   0x00

// Macros
#define EDX_REGS_ON EDX_REG_KEY = EDX_REG_KEY_UNLOCK
#define EDX_REGS_OFF EDX_REG_KEY = EDX_REG_KEY_LOCK

#endif /* _EVERDRIVEX_H_DEFINED */
