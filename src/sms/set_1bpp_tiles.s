        .include        "global.s"

        .ez80

        .globl __current_1bpp_colors

        .area   _HOME

; void set_1bpp_data(uint8_t *first_tile, uint16_t nb_tiles, const uint8_t *data) __sdcccall(0) __z88dk_callee;
_set_1bpp_data::
        DISABLE_VBLANK_COPY     ; switch OFF copy shadow SAT

        pop de                  ; pop ret address
        pop iy

        ex de, hl               ; hl = ret

        pop bc                  ; bc = ntiles
        ex (sp), hl             ; hl = src

        inc b
        inc c

        ld de, (__current_1bpp_colors)

        ld a, e
        and #0x0f
        .rept 4
            sla d
        .endm
        or d
        ld e, a                 ; e = color data

0$:
        VDP_WRITE_CMD iyh, iyl

        nop
        ld d, #8
1$:
        ld a, (hl)
        inc hl
        out (.VDP_DATA), a
        dec d
        jr nz, 1$

        ld a, iyh
        or #0x20
        ld iyh, a
        VDP_WRITE_CMD iyh, iyl

        nop
        ld d, #8
2$:
        ld a, e
        out (.VDP_DATA), a
        dec d
        jr nz, 2$

        ld a, #8
        add iyl
        ld iyl, a
        adc iyh
        sub iyl
        and #~0x20
        ld iyh, a

        dec c
        jr  nz, 0$

        dec b
        jr  nz, 0$

        ENABLE_VBLANK_COPY        ; switch ON copy shadow SAT

        ret
