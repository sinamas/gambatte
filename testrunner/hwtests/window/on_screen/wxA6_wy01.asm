.size 8000

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
	ld hl, 8000
	xor a, a
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
	dec a
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
	ld hl, 9800
	ld a, 00
	ld b, 03
lbegin_fill_bgmap0:
	ld c, 00
lbegin_fill_bgmap0_inner:
	ld(hl++), a
	dec c
	jrnz lbegin_fill_bgmap0_inner
	dec b
	jrnz lbegin_fill_bgmap0
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
	ld hl, 9c00
	ld a, 00
	ld de, 0021
	ld b, 20
lbegin_set_bgmap1_start00skip21:
	ld(hl), a
	add hl, de
	dec b
	jrnz lbegin_set_bgmap1_start00skip21
	ld a, 03
	ldff(47), a
	ld a, 01
	ldff(4a), a
	ld a, a6
	ldff(4b), a
	ld a, f1
	ldff(40), a
limbo:
	jr limbo

