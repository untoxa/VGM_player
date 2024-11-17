/*-------------------------------------------------------------------------*/
/* PFF - Low level disk control module for EZFlash Junior (C)asie, 2024    */
/*-------------------------------------------------------------------------*/

#include <gbdk/platform.h>
#include <string.h>

#include "diskio.h"
#include "ezflashjr.h"

uint32_t CachedSector;

inline void ezjr_unlock(void) {
    EZJR_REG_UNLOCK1 = EZJR_UNLOCK1;
    EZJR_REG_UNLOCK2 = EZJR_UNLOCK2;
    EZJR_REG_UNLOCK3 = EZJR_UNLOCK3;
}

inline void ezjr_lock(void) {
    EZJR_REG_LOCK = EZJR_LOCK;
}

DSTATUS disk_initialize(void) {
    CachedSector = 0xffffffff;

    ezjr_unlock();

    EZJR_REG_31 = 0x00;
    EZJR_REG_32 = 0x80;

    ezjr_lock();

    return 0;
}

DRESULT disk_readp(
    uint8_t *buff,      /* Pointer to the read buffer (NULL:Forward to the stream) */
    uint32_t sector,    /* Sector number (LBA) */
    uint16_t offset,    /* Byte offset to read from (0..511) */
    uint16_t count      /* Number of bytes to read (ofs + cnt mus be <= 512) */
) {
    DRESULT res = RES_ERROR;

    ezjr_unlock();
    EZJR_REG_SRAM_MAP = EZJR_SRAM_MAP_NONE;

    if (CachedSector != sector) {
        EZJR_REG_TF_SRAM_MAP = EZJR_TF_SRAM_MAP_NONE;

        EZJR_REG_TF_SECTOR = sector;
        EZJR_REG_TF_COMMAND = EZJR_TF_COMMAND_READ(1);

        EZJR_REG_TF_SRAM_MAP = EZJR_TF_SRAM_MAP_STATUS;
        while (_SRAM[0] & EZJR_TF_STATUS_BUSY);

        CachedSector = sector;
    }

    EZJR_REG_TF_SRAM_MAP = EZJR_TF_SRAM_MAP_DATA;
    memcpy(buff, _SRAM + offset, count);

    EZJR_REG_TF_SRAM_MAP = EZJR_TF_SRAM_MAP_NONE;
    EZJR_REG_SRAM_MAP = EZJR_SRAM_MAP_SRAM;
    ezjr_lock();

    return RES_OK;
}

#if PF_USE_WRITE
DRESULT disk_writep (
    const uint8_t *buff,    /* Pointer to the bytes to be written (NULL:Initiate/Finalize sector write) */
    uint32_t sc             /* Number of bytes to send, Sector number (LBA) or zero */
) {
    static uint16_t wc; /* Sector write counter */

    ezjr_unlock();
    EZJR_REG_SRAM_MAP = EZJR_SRAM_MAP_NONE;

    CachedSector = 0xffffffff;

    if (buff) {     /* Send data bytes */
        EZJR_REG_TF_SRAM_MAP = EZJR_TF_SRAM_MAP_DATA;
        memcpy(_SRAM + wc, buff, sc);
        wc += sc;
    } else {
        EZJR_REG_TF_SRAM_MAP = EZJR_TF_SRAM_MAP_NONE;

        if (sc) {   /* Initiate sector write process */
            EZJR_REG_TF_SECTOR = sc;
            wc = 0;
        } else {    /* Finalize sector write process */
            EZJR_REG_TF_COMMAND = EZJR_TF_COMMAND_WRITE(1);

            EZJR_REG_TF_SRAM_MAP = EZJR_TF_SRAM_MAP_STATUS;
            while (_SRAM[0] & EZJR_TF_STATUS_BUSY);
        }
    }

    EZJR_REG_TF_SRAM_MAP = EZJR_TF_SRAM_MAP_NONE;
    EZJR_REG_SRAM_MAP = EZJR_SRAM_MAP_SRAM;
    ezjr_lock();
    return RES_OK;
}
#endif
