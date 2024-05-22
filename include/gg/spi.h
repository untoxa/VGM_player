#ifndef _SPI_H_DEFINED
#define _SPI_H_DEFINED

#include <gbdk/platform.h>
#include <stdint.h>

#include "everdrive.h"

#define CS_HIGH()    (CFG_REG &= ~(1 << _CFG_SPI_SS))  /* Set CS high */
#define CS_LOW()     (CFG_REG |=  (1 << _CFG_SPI_SS))  /* Set CS low */
#define IS_CS_LOW    (CFG_REG &   (1 << _CFG_SPI_SS))  /* Test if CS is low */

#define dly_100us() delay(1);        /* usi.S: Delay 100 microseconds */
#define rcv_spi() xmit_spi(0xff)     /* usi.S: Send a 0xFF to the MMC and get the received byte */

inline void init_spi (void) {
    RAM_REGS_ON;
    AUTO_BUSY_OFF;
}

inline void on_disk_init (void) {
    SPI_SPEED_OFF;
}

uint8_t xmit_spi(int8_t spi_data) PRESERVES_REGS(b, c, d, e, iyh, iyl);

inline void disk_access_start (void) {
    ROM_REGS_ON;
}

inline void disk_access_finish (void) {
    ROM_REGS_OFF;
}

#endif  /* _SPI_H_DEFINED */
