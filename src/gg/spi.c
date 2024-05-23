#include <gbdk/platform.h>

#include "spi.h"

uint8_t xmit_spi(int8_t spi_data) PRESERVES_REGS(b, c, d, e, iyh, iyl) NAKED {
    spi_data;
    __asm
        ld  (_SPI_PORT), a
        ld  hl, #_STATE_PORT
1$:
        bit 7, (hl)
        jr  nz, 1$
        ld  a, (_SPI_PORT)
        ret
    __endasm;
}
