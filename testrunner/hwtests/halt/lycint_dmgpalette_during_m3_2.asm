.size 8000

.text@48
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
	ld a, 40
	ldff(c), a
	ld a, 02
	ldff(ff), a
	xor a, a
	ldff(0f), a
	ldff(45), a
	ld c, 47
	ld a, 00
	ldff(c), a
	dec a
	ld b, 90
	ei
	halt

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
	inc a

.text@1031
	ldff(c), a
	pop hl
	ldff a, (45)
	inc a
	cmp a, b
	jrnz lstatint_set_lyc
	xor a, a
lstatint_set_lyc:
	ldff(45), a
	ld a, ff
	ei
	halt

