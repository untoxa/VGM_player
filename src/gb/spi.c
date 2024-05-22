#include <gbdk/platform.h>

#include "spi.h"

uint8_t xmit_spi(int8_t spi_data) PRESERVES_REGS(b, c, d, e) NAKED {
    spi_data;
    __asm
        ld  (_EDX_REG_SPI_DATA), a
        ld  hl, #_EDX_REG_SPI_CTRL
1$:
        bit 7, (hl)
        jr  nz, 1$
        ld  a, (_EDX_REG_SPI_DATA)
        ret
    __endasm;
}
