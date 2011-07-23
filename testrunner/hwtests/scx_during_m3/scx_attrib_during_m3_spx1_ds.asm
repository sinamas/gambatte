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
	ld hl, 8000
	ld b, 08
l170:
	ld a, 00
	ld(hl++), a
	ld a, 7e
	ld(hl++), a
	dec b
	jrnz l170
	ld hl, 8000
	ld a, 01
	ldff(4f), a
	ld b, 08
l182:
	ld a, ff
	ld(hl++), a
	ld a, 81
	ld(hl++), a
	dec b
	jrnz l182
	ld c, 12
	ld hl, 9800
l190:
	ld b, 06
	ld a, 08
l194:
	ld(hl++), a
	ld(hl++), a
	dec b
	jrnz l194
	ld b, 0a
	xor a, a
l19c:
	ld(hl++), a
	ld(hl++), a
	dec b
	jrnz l19c
	dec c
	jrnz l190
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
	ld hl, fe00
	ld b, a0
	xor a, a
l1c7:
	ld(hl++), a
	dec b
	jrnz l1c7
	ld a, 10
	ld(fe00), a
	ld a, 01
	ld(fe01), a
	ld a, 20
	ldff(41), a
	ld a, 02
	ldff(ff), a
	ld c, 43
	ld a, 93
	ldff(40), a
	ei
	xor a, a

.code@1000
l1000:
	ldff(c), a
	ld a, 60

.code@1024
	ldff(c), a
	pop hl
	ld a, c0

.code@1064
	ldff(c), a
	xor a, a

