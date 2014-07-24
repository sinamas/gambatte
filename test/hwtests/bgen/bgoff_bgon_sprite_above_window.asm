.size 8000

.text@48
	pop de
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
	ld c, 69
	xor a, a
	ldff(c), a
	ldff(c), a
	ld a, 86
	ldff(68), a
	ld a, ff
	ldff(c), a
	ldff(c), a
	ld a, 80
	ldff(6a), a
	ld c, 6b
	ld a, 55
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ld hl, 8000
	ld a, aa
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
	ld a, 55
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
	xor a, a
	ld hl, fea0
lbegin_fill_oam:
	dec l
	ld(hl), a
	jrnz lbegin_fill_oam
	ld a, 10
	ld(hl), a
	inc l
	ld a, 11
	ld(hl), a
	inc l
	ld a, 01
	ld(hl), a
	inc l
	ld a, 00
	ld(hl), a
	inc l
	ld a, 78
	ld(hl), a
	inc l
	ld a, 92
	ld(hl), a
	inc l
	ld a, 01
	ld(hl), a
	inc l
	ld a, 00
	ld(hl), a
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
	ld hl, 9c10
	ld a, 02
	ld de, 0021
	ld b, 20
lbegin_set_bgmap1_start10skip21:
	ld(hl), a
	add hl, de
	dec b
	jrnz lbegin_set_bgmap1_start10skip21
	ld a, 03
	ldff(47), a
	ld a, 55
	ldff(48), a
	ld a, 18
	ldff(4a), a
	ld a, 0f
	ldff(4b), a
	ld a, f7
	ldff(40), a
	ld a, 20
	ldff(41), a
	xor a, a
	ldff(0f), a
	ld a, 02
	ldff(ff), a
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
	ld a, f6
	ldff(40), a

.text@102d
	ld a, f7
	ldff(40), a
	ei

