.size 8000

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
	ld hl, 8010
	ld a, ff
	ld(hl++), a
	ld(hl++), a
	ld a, 01
	ld b, 32
	ld hl, 9a40
l175:
	ld(hl++), a
	dec b
	jpnz l175
	ld a, 80
	ldff(68), a
	ld a, ff
	ld c, 69
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	xor a, a
	ldff(c), a
	ldff(c), a
	nop
	ld c, 41
	ld b, 03
l190:
	ldff a, (c)
	and a, b
	cmp a, b
	jpnz l190
	xor a, a
	ldff(45), a
	ld a, 40
	ldff(c), a
	ld a, 02
	ldff(ff), a
	ld c, 42
	ld b, 90
l1a4:
	halt
	xor a, a
	ldff(0f), a
	jp l1000

.code@1000
l1000:
	nop

.code@1018
	ldff a, (44)
	ld d, a
	ld a, b
	sub a, d
	ldff(c), a
	xor a, a

.code@1069
	ldff(c), a
	ld a, d
	inc a
	cmp a, b
	jrnz l1070
	xor a, a
l1070:
	ldff(45), a
	jp l1a4

