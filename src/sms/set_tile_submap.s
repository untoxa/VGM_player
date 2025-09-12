        .include        "global.s"

        .title  "VRAM utilities"
        .module VRAMUtils

        .ez80

        .area   _DATA

.image_tile_width::
        .ds     0x01

        .area   _HOME

        .VDP_TILEMAP = 0x5C00

; void set_tile_submap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t map_w, const uint8_t *map) __z88dk_callee;
_set_tile_submap::
        pop iy                  ; IY = ret
        pop bc                  ; BC = YX
        pop hl                  ; HL = HW
        pop de                  ; DE = data

        dec sp
        pop af
        sub l
        ld (.image_tile_width), a ; .image_tile_width contains corrected width

        push iy

        push hl
        push de

        add l
        ld e, a
        ld d, #0
        ld a, b
        MUL_DE_BY_A_RET_HL
        ld a, c
        ADD_A_REG16 h, l

        pop de
        add hl, de
        ex de, hl

        pop hl

        ld a, b
        ld b, d

        add #.SCREEN_Y_OFS
        ld d, a
        xor a
        FAST_MOD8 d #24
        ld d, a

        ld a, c
        add #.SCREEN_X_OFS
        and #0x1f
        ld c, e
        ld e, a                 ; BC = data, DE = YX

        ;; Set background tile table from (BC) at XY = DE of size WH = HL
.set_tile_submap_xy::
        push hl
        ld hl, #.VDP_TILEMAP

        ;; Set background tile from (BC) at YX = DE, size WH on stack, to VRAM from address (HL)
.set_tile_submap_xy_tt::
        ld a, h
        ld iyh, a
        push bc                 ; Store source

        ld a, d
        rrca                    ; rrca(3) == rlca(5)
        rrca
        rrca
        ld d, a
        and #0x03
        add h
        ld b, a
        ld a, #0xE0
        and d
        add e
        ld c, a                 ; dest BC = HL + ((0x20 * Y) * 2) + (X * 2)

        ld a, b
        and #0b00000111
        or iyh
        ld b, a

        DISABLE_VBLANK_COPY     ; switch OFF copy shadow SAT

        pop hl                  ; HL = source
        pop de                  ; DE = HW
        push ix                 ; save IX
        push de                 ; store HW

        ld ixh, b
        ld ixl, c
        push ix                 ; store dest

1$:                             ; copy H rows
        VDP_WRITE_CMD ixh, ixl
        ld c, #.VDP_DATA
2$:                             ; copy W tiles
        ld a, (__map_tile_offset)
        add (hl)
        out (c), a
        inc hl
        dec b

        ld a, ixl
        and #0x3F
        inc a
        inc a
        bit 6, a
        jp z, 3$
        and #0x3F
        ld b, a
        ld a, ixl
        and #0xC0
        or b
        ld ixl, a
        VDP_WRITE_CMD ixh, ixl
        dec e
        jp nz, 2$
        jp 7$
3$:
        inc ixl
        inc ixl
        dec e
        jp nz, 2$
7$:
        pop ix
        pop de

        dec d
        jr z, 6$

        push de

        ld a, (.image_tile_width)
        ADD_A_REG16 h, l

        ld bc, #0x40
        add ix, bc

        ld a, ixh
        and #0b00000111
        cp #0x07
        jp nz, 9$
        xor a
9$:
        or iyh
        ld ixh, a

        push ix
        jp 1$
6$:
        ENABLE_VBLANK_COPY      ; switch ON copy shadow SAT
        pop ix                  ; restore IX
        ret
