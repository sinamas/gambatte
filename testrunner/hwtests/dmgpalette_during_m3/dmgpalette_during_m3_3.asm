.size 8000

.code@48
	ei
	jp lstatint

.code@100
	jp lbegin

.code@150
lbegin:
	ld c, 41
	ld b, 03
lbegin_waitm3:
	ldff a, (c)
	and a, b
	cmp a, b
	jpnz lbegin_waitm3
	ld a, 20
	ldff(c), a
	ld a, 02
	ldff(ff), a
	xor a, a
	ldff(0f), a
	ei
	ld c, 47
	ld a, ff
	ldff(c), a
	inc a

.code@1000
lstatint:
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
	ldff(c), a
	dec a

.code@1032
	ldff(c), a
	inc a
	pop hl

