#ifndef _SPI_H_DEFINED
#define _SPI_H_DEFINED

#include <gbdk/platform.h>
#include <stdint.h>

#define CS_LOW()           /* Set CS low */
#define CS_HIGH()          /* Set CS high */
#define IS_CS_LOW   (0)    /* Test if CS is low */

#define dly_100us() delay(1);        /* usi.S: Delay 100 microseconds */
#define rcv_spi() xmit_spi(0xff)     /* usi.S: Send a 0xFF to the MMC and get the received byte */

inline void init_spi (void) {
}

inline void on_disk_init (void) {
}

uint8_t xmit_spi(int8_t spi_data);

inline void disk_access_start (void) {
}

inline void disk_access_finish (void) {
}

#endif  /* _SPI_H_DEFINED */
