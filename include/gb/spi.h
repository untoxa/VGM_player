#ifndef _SPI_H_DEFINED
#define _SPI_H_DEFINED

#include <gbdk/platform.h>
#include <stdint.h>

#include "everdrivex.h"

#define CS_HIGH()    (EDX_REG_SPI_CTRL |=  (1 << EDX_REG_SPI_SS))  /* Set CS high */
#define CS_LOW()     (EDX_REG_SPI_CTRL &= ~(1 << EDX_REG_SPI_SS))  /* Set CS low */
#define IS_CS_LOW  (!(EDX_REG_SPI_CTRL &   (1 << EDX_REG_SPI_SS))) /* Test if CS is low */

#define dly_100us() delay(1);        /* usi.S: Delay 100 microseconds */
#define rcv_spi() xmit_spi(0xff)     /* usi.S: Send a 0xFF to the MMC and get the received byte */

inline void init_spi (void) {
    EDX_REG_SPI_CTRL |= (1 << EDX_REG_SPI_LOW_SPEED);
}

inline void on_disk_init (void) {
    EDX_REG_SPI_CTRL &= ~(1 << EDX_REG_SPI_LOW_SPEED);
}

uint8_t xmit_spi(int8_t spi_data) PRESERVES_REGS(b, c, d, e);

inline void disk_access_start (void) {
    EDX_REGS_ON;
}

inline void disk_access_finish (void) {
    EDX_REGS_OFF;
}

#endif  /* _SPI_H_DEFINED */
