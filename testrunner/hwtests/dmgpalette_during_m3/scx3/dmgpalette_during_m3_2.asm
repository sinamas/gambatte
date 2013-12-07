.size 8000

.text@48
	ei
	jp lstatint

.text@100
	jp lbegin

.text@150
lbegin:
	ld c, 41
	ld b, 03
lbegin_waitm3:
	ldff a, (c)
	and a, b
	cmp a, b
	jrnz lbegin_waitm3
	ld a, 20
	ldff(c), a
	ld a, 02
	ldff(ff), a
	xor a, a
	ldff(0f), a
	ei
	ld c, 47
	ld a, 03
	ldff(43), a
	ld a, ff
	ldff(c), a
	inc a

.text@1000
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
	ldff(c), a
	dec a

.text@1033
	ldff(c), a
	inc a
	pop hl

