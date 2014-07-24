.size 8000

.text@48
	ei
	jp lstatint

.text@100
	jp lbegin

.data@143
	80

.text@150
lbegin:
	ld c, 44
	ld b, 90
lbegin_waitvblank:
	ldff a, (c)
	cmp a, b
	jrnz lbegin_waitvblank
	xor a, a
	ldff(40), a
	ld hl, 9f00
	ld b, 20
lbegin_clearvram:
	dec l
	ld(hl), a
	jrnz lbegin_clearvram
	dec h
	dec b
	jrnz lbegin_clearvram
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
lbegin_fill_oam:
	dec l
	ld(hl), a
	jrnz lbegin_fill_oam
	ld a, 10
	ld(hl), a
	inc l
	ld a, 0b
	ld(hl), a
	ld a, 87
	ldff(40), a
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
	nop
	nop
	ld a, 97
	ldff(40), a
	pop hl

.text@1028
	ld a, 87
	ldff(40), a

