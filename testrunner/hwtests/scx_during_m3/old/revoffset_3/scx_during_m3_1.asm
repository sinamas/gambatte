.size 8000

.code@48
	ei
	jp lstatint

.code@100
	jp lbegin

.code@150
lbegin:
	ld c, 44
	ld b, 90
lbegin_waitvblank:
	ldff a, (c)
	cmp a, b
	jpnz lbegin_waitvblank
	ld a, 11
	ldff(40), a
	ld hl, 8010
	ld a, ff
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld c, 12
	ld de, 0014
	ld hl, 9800
	ld a, 01
lbegin_set_bgmap:
	ld b, 0c
lbegin_set_bgmapline_tilenos0to11:
	ld(hl++), a
	dec b
	jpnz lbegin_set_bgmapline_tilenos0to11
	add hl, de
	dec c
	jpnz lbegin_set_bgmap
	ld a, 03
	ldff(47), a
	ld a, 20
	ldff(41), a
	ld a, 02
	ldff(ff), a
	ld c, 43
	ld a, 91
	ldff(40), a
	ei
	xor a, a

.code@1000
lstatint:
	ldff(c), a
	ld a, 63
	nop
	nop
	nop
	nop
	nop
	ldff(c), a
	pop hl
	ld a, c0

.code@102d
	ldff(c), a
	xor a, a

