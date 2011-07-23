.size 8000

.code@48
	ei
	jp l1000

.code@100
	jp l150

.code@150
l150:
	ld c, 41
	ld b, 03
l154:
	ldff a, (c)
	and a, b
	cmp a, b
	jpnz l154
	ld a, 20
	ldff(c), a
	ld a, 02
	ldff(ff), a
	xor a, a
	ldff(0f), a
	ei
	ld c, 47
	ld a, 02
	ldff(43), a
	ld a, ff
	ldff(c), a
	inc a

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
	ldff(c), a
	dec a

.code@1034
	ldff(c), a
	inc a
	pop hl

