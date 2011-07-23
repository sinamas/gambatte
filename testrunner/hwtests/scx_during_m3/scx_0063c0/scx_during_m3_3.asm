.size 8000

.code@48
	ei
	jp l1000

.code@100
	jp l150

.data@143
	80 

.code@150
l150:
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	ld c, 44
	ld b, 90
l162:
	ldff a, (c)
	cmp a, b
	jpnz l162
	ld a, 11
	ldff(40), a
	ld hl, 8000
	ld b, 08
l170:
	ld a, 00
	ld(hl++), a
	ld a, 7e
	ld(hl++), a
	dec b
	jrnz l170
	ld b, 08
l17b:
	ld a, 00
	ld(hl++), a
	ld a, 81
	ld(hl++), a
	dec b
	jrnz l17b
	ld b, 08
l186:
	ld a, ff
	ld(hl++), a
	ld a, 81
	ld(hl++), a
	dec b
	jrnz l186
	ld b, 08
l191:
	ld a, ff
	ld(hl++), a
	ld a, 7e
	ld(hl++), a
	dec b
	jrnz l191
	ld c, 12
	ld hl, 9800
l19f:
	ld b, 06
	ld a, 02
l1a3:
	ld(hl++), a
	inc a
	ld(hl++), a
	dec a
	dec b
	jrnz l1a3
	ld b, 0a
l1ac:
	xor a, a
	ld(hl++), a
	inc a
	ld(hl++), a
	dec b
	jrnz l1ac
	dec c
	jrnz l19f
	ld a, 27
	ldff(47), a
	ld a, 80
	ldff(68), a
	ld c, 69
	xor a, a
	ldff(c), a
	ldff(c), a
	ld a, 94
	ldff(c), a
	ld a, 52
	ldff(c), a
	ld a, 08
	ldff(c), a
	ld a, 21
	ldff(c), a
	ld a, ff
	ldff(c), a
	ldff(c), a
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
	nop
	ldff(c), a
	pop hl
	ld a, c0

.code@102b
	ldff(c), a
	xor a, a

