.size 8000

.text@48
	jp lstatint

.text@100
	jp lbegin

.text@150
lbegin:
	ld a, ff
	ldff(45), a
	ld c, 44
	ld b, 91
lbegin_waitly91:
	ldff a, (c)
	cmp a, b
	jrnz lbegin_waitly91
	xor a, a
	ldff(40), a
	ld hl, 8000
lbegin_fillvramff:
	ld a, ff
	ld(hl++), a
	ld a, a0
	cmp a, h
	jrnz lbegin_fillvramff
	ld a, ff
	ldff(47), a
	ld a, 55
	ldff(49), a
	xor a, a
	ldff(48), a
	ld hl, fea0
lbegin_fill_oam:
	dec l
	ld(hl), a
	jrnz lbegin_fill_oam
	ld hl, fe98
	ld d, 10
	ld a, d
	ld(hl), a
	inc l
	ld a, 08
	ld(hl), a
	inc l
	inc l
	ld a, d
	ld(hl), a
	inc l
	ld(hl), a
	inc l
	ld a, 0c
	ld(hl), a
	ld a, 97
	ldff(40), a
lbegin_waitly91_2:
	ldff a, (c)
	cmp a, b
	jrnz lbegin_waitly91_2
	ld a, 20
	ldff(41), a
	ld a, 02
	ldff(ff), a
	xor a, a
	ldff(0f), a
	ei
	ld c, 40
	ld d, 97

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
	nop
	nop
	nop
	ld a, 95
	ldff(c), a
	ld a, d
	ldff(c), a
	ld sp, fffe
	ei

