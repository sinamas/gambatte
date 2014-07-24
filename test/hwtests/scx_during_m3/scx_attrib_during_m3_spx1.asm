.size 8000

.text@48
	ei
	jp lstatint

.text@100
	jp lbegin

.data@143
	c0

.text@150
lbegin:
	ld c, 44
	ld b, 90
lbegin_waitly90:
	ldff a, (c)
	cmp a, b
	jrnz lbegin_waitly90
	ld a, 11
	ldff(40), a
	ld hl, 8000
	ld b, 08
lbegin_setbank0tile0data:
	ld a, 00
	ld(hl++), a
	ld a, 7e
	ld(hl++), a
	dec b
	jrnz lbegin_setbank0tile0data
	ld hl, 8000
	ld a, 01
	ldff(4f), a
	ld b, 08
lbegin_setbank1tile0data:
	ld a, ff
	ld(hl++), a
	ld a, 81
	ld(hl++), a
	dec b
	jrnz lbegin_setbank1tile0data
	ld c, 12
	ld hl, 9800
lbegin_set_bgmap_attrs:
	ld b, 06
	ld a, 08
lbegin_set_bgmapline_attrs0to11:
	ld(hl++), a
	ld(hl++), a
	dec b
	jrnz lbegin_set_bgmapline_attrs0to11
	ld b, 0a
	xor a, a
lbegin_set_bgmapline_attrs12to31:
	ld(hl++), a
	ld(hl++), a
	dec b
	jrnz lbegin_set_bgmapline_attrs12to31
	dec c
	jrnz lbegin_set_bgmap_attrs
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
lbegin_clear_oam:
	ld(hl++), a
	dec b
	jrnz lbegin_clear_oam
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

.text@1000
lstatint:
	ldff(c), a
	ld a, 60
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	ldff(c), a
	pop hl
	ld a, c0

.text@102a
	ldff(c), a
	xor a, a

