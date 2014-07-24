.size 8000

.text@100
	jp lbegin

.text@150
lbegin:
	ld c, 44
	ld b, 90
lbegin_waitvblank:
	ldff a, (c)
	cmp a, b
	jrnz lbegin_waitvblank
	ld hl, 8010
	ld a, ff
	ld(hl++), a
	ld(hl++), a
	ld a, 01
	ld b, 32
	ld hl, 9a40
lbegin_settilemap:
	ld(hl++), a
	dec b
	jrnz lbegin_settilemap
	ld a, c0
	ldff(47), a
	ld c, 41
	ld b, 03
lbegin_waitm3:
	ldff a, (c)
	and a, b
	cmp a, b
	jrnz lbegin_waitm3
	xor a, a
	ldff(45), a
	ld a, 40
	ldff(c), a
	ld a, 02
	ldff(ff), a
	ld c, 42
	ld b, 90
lwait_lycirq:
	halt
	xor a, a
	ldff(0f), a
	jp llycint

.text@1000
llycint:
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	ldff a, (44)
	ld d, a
	ld a, b
	sub a, d
	ldff(c), a
	xor a, a

.text@102c
	ldff(c), a
	ld a, d
	inc a
	cmp a, b
	jrnz llycint_waitlya
	xor a, a
llycint_waitlya:
	ldff(45), a
	jp lwait_lycirq

