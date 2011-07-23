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
	ld c, 44
	ld b, 90
l154:
	ldff a, (c)
	cmp a, b
	jpnz l154
	xor a, a
	ldff(40), a
	ld hl, 9f00
	ld b, 20
l161:
	dec l
	ld(hl), a
	jrnz l161
	dec h
	dec b
	jrnz l161
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
	ld(hl++), a
	ld(hl++), a
	ld hl, 9f00
	ld b, 04
	ld a, 01
l187:
	dec l
	ld(hl), a
	jrnz l187
	dec h
	dec b
	jrnz l187
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
l1c0:
	dec l
	ld(hl), a
	jrnz l1c0
	ld a, 10
	ld(hl), a
	inc l
	ld a, 08
	ld(hl), a
	ld a, 97
	ldff(40), a
	ld c, 41
	ld b, 03
l1d3:
	ldff a, (c)
	and a, b
	cmp a, b
	jpnz l1d3
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
	ld a, 9f
	ldff(40), a
	pop hl

.code@1029
	ld a, 97
	ldff(40), a

