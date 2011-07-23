.size 8000

.code@48
	ei
	jp l1000

.code@100
	jp l150

.code@150
l150:
	ld c, 44
	ld b, 90
l154:
	ldff a, (c)
	cmp a, b
	jpnz l154
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
l17c:
	ld b, 0c
l17e:
	ld(hl++), a
	dec b
	jpnz l17e
	add hl, de
	dec c
	jpnz l17c
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
l1000:
	ldff(c), a
	ld a, 63
	nop
	nop
	nop
	nop
	nop
	nop
	ldff(c), a
	pop hl
	ld a, c0

.code@102c
	ldff(c), a
	xor a, a

