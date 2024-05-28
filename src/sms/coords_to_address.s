        .include        "global.s"

        .title  "VRAM utilities"
        .module VRAMUtils

        .area   _HOME

        .VDP_TILEMAP = 0x5C00

; translate coords in DE into address in DE
.coords_to_address::
        ld b, #>.VDP_TILEMAP
	
        ld a, d
        add #.SCREEN_Y_OFS
        ld d, a
        xor a
        ld c, a
        FAST_MOD8 d #24
        ld d, a

        ld a, e
        and #0x1f
        ld e, a

        ld a, d
        rrca                    ; rrca(2) == rlca(6)
        rrca
        rrca
        ld d, a
        and #0x03
        add b
        ld b, a
        ld a, #0xE0
        and d
        add e
        ld e, a
        ld d, b                 ; dest DE = BC + ((0x20 * Y) * 2) + (X * 2)

        ret

; uint8_t * get_bkg_xy_addr(uint8_t x, uint8_t y) __z88dk_callee __preserves_regs(iyh, iyl);

_get_bkg_xy_addr::
        pop hl
        ex (sp), hl
        ex de, hl
        call .coords_to_address
        ex de, hl
        ret