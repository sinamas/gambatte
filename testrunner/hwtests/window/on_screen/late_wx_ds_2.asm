.size 8000

.text@48
	jp lstatint

.text@100
	jp lbegin

.data@143
	c0

.text@150
lbegin:
	ld a, 00
	ldff(ff), a
	ld a, 30
	ldff(00), a
	ld a, 01
	ldff(4d), a
	stop, 00
	ld c, 44
	ld b, 91
lbegin_waitly91:
	ldff a, (c)
	cmp a, b
	jrnz lbegin_waitly91
	xor a, a
	ldff(40), a
	ld a, 80
	ldff(68), a
	xor a, a
	ldff(69), a
	ldff(69), a
	ld a, 86
	ldff(68), a
	ld a, ff
	ldff(69), a
	ldff(69), a
	ld hl, 8010
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
	ld hl, 9c00
	ld a, 01
	ld b, 03
lbegin_fill_bgmap1:
	ld c, 00
lbegin_fill_bgmap1_inner:
	ld(hl++), a
	dec c
	jrnz lbegin_fill_bgmap1_inner
	dec b
	jrnz lbegin_fill_bgmap1
	ld a, 07
	ldff(4b), a
	ld a, f1
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
	ei

.text@1000
lstatint:
	nop

.text@1022
	ld a, ff
	ldff(4b), a

.text@1076
	ld a, 07
	ldff(4b), a
	pop de
	ei

