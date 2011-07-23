.size 8000

.code@48
	ei
	jp l1000

.code@100
	jp l150

.data@143
	c0 

.code@150
l150:
	ld a, 00
	ldff(ff), a
	ld a, 30
	ldff(00), a
	ld a, 01
	ldff(4d), a
	stop, 00
	ld c, 44
	ld b, 90
l162:
	ldff a, (c)
	cmp a, b
	jpnz l162
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
l18a:
	ld b, 0c
l18c:
	ld(hl++), a
	dec b
	jpnz l18c
	add hl, de
	dec c
	jpnz l18a
	ld a, 80
	ldff(68), a
	xor a, a
	ld c, 69
	ldff(c), a
	ldff(c), a
	dec a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	nop
	ld a, 20
	ldff(41), a
	ld a, 02
	ldff(ff), a
	ld c, 43
	ld a, 91
	ldff(40), a
	ei
	ld a, 03

.code@1000
l1000:
	ldff(c), a
	ld a, 60

.code@1020
	ldff(c), a
	pop hl
	ld a, c0

.code@1068
	ldff(c), a
	ld a, 03

