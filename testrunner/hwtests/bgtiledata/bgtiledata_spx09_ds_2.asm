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
	xor a, a
	ldff(40), a
	ld hl, 9f00
	ld b, 20
l16f:
	dec l
	ld(hl), a
	jrnz l16f
	dec h
	dec b
	jrnz l16f
	ld hl, 8000
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
	ld(hl++), a
	ld(hl++), a
	ld a, e4
	ldff(47), a
	ld a, ff
	ldff(48), a
	ld a, 80
	ldff(68), a
	ld a, ff
	ld c, 69
	ldff(c), a
	ldff(c), a
	ld a, aa
	ldff(c), a
	ldff(c), a
	ld a, 55
	ldff(c), a
	ldff(c), a
	xor a, a
	ldff(c), a
	ldff(c), a
	ld a, 80
	ldff(6a), a
	ld a, 00
	ld c, 6b
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ld hl, fea0
	xor a, a
l1bf:
	dec l
	ld(hl), a
	jrnz l1bf
	ld a, 10
	ld(hl), a
	inc l
	ld a, 09
	ld(hl), a
	ld a, 87
	ldff(40), a
	ld c, 41
	ld b, 03
l1d2:
	ldff a, (c)
	and a, b
	cmp a, b
	jpnz l1d2
	ld a, 20
	ldff(c), a
	ld a, 02
	ldff(ff), a
	xor a, a
	ldff(0f), a
	ei

.code@1000
l1000:
	nop

.code@102d
	ld a, 97
	ldff(40), a
	pop hl

.code@1066
	ld a, 87
	ldff(40), a

