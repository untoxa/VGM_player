        .include        "global.s"

        .title  "VRAM utilities"
        .module VRAMUtils

        .globl  .coords_to_address

        .area   _HOME

; void set_vram_byte(uint8_t * addr, uint8_t v) __z88dk_callee __preserves_regs(iyh, iyl);
_set_vram_byte::
        DISABLE_VBLANK_COPY

        pop de
        pop hl
        WRITE_VDP_CMD_HL
        ex de, hl
        dec sp
        ex (sp), hl
        ld a, h
        out (.VDP_DATA), a

        ENABLE_VBLANK_COPY
        ret

; uint8_t * set_tile_xy(uint8_t x, uint8_t y, uint8_t t) __z88dk_callee __preserves_regs(iyh, iyl);
_set_tile_xy::
        DISABLE_VBLANK_COPY

        pop hl          ; HL = ret
        pop de          ; DE = YX
        dec sp
        ex (sp), hl     ; HL = data

        call .coords_to_address
        ex de, hl

        WRITE_VDP_CMD_HL

        ld a, d
        out (.VDP_DATA), a

        ex de, hl
        ENABLE_VBLANK_COPY
        ex de, hl
        ret
