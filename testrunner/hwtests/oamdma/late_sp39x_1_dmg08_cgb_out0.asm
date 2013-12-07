.size 8000

.data@0
	01

.text@48
	jp ff81

.data@9c
	02 03 04 05

.text@100
	jp lbegin

.data@143
	80 00 00 00 1a 00 03

.text@150
lbegin:
	ld de, 1028
	ld hl, ffa8
lbegin_copydmaroutine:
	dec e
	ld a, (de)
	ld(hl--), a
	jrnz lbegin_copydmaroutine
	ld b, 90
	call lwaitly_b
	ld a, 0a
	ld(0000), a
	ld hl, fea0
	ld a, a0
lbegin_fill_oam:
	dec l
	ld(hl), a
	jrnz lbegin_fill_oam
	ld b, 90
	call lwaitly_b
	ld hl, c09f
	ld c, a0
	ld a, 10
lbegin_fillwram:
	dec c
	ld(hl--), a
	jrnz lbegin_fillwram
	ld a, 10
	ld(fe9c), a
	ld a, 02
	ldff(45), a
	ld a, 40
	ldff(41), a
	xor a, a
	ldff(0f), a
	ld a, 02
	ldff(ff), a
	ei
	ld a, 93
	ldff(40), a

.text@1000
	ld c, 1d
l1002:
	dec c
	jrnz l1002
	nop
	ld a, c0
	ldff(46), a
	ld c, 0a
l100c:
	dec c
	jrnz l100c
	nop
	nop
	nop
	ldff a, (41)
	ld b, 03
	and a, b
	ldff(80), a
	ld c, 28
l101b:
	dec c
	jrnz l101b
	jp lprintff80

.text@7000
lprintff80:
	ld b, 91
	call lwaitly_b
	xor a, a
	ldff(40), a
	ld bc, 7a00
	ld hl, 8000
	ld d, a0
lprintff80_copytiles:
	ld a, (bc)
	inc bc
	ld(hl++), a
	dec d
	jrnz lprintff80_copytiles
	ld a, c0
	ldff(47), a
	ld a, 80
	ldff(68), a
	ld a, ff
	ldff(69), a
	ldff(69), a
	ldff(69), a
	ldff(69), a
	ldff(69), a
	ldff(69), a
	ld a, 00
	ldff(69), a
	ldff(69), a
	ldff a, (80)
	ld(9800), a
	xor a, a
	ldff(43), a
	ld a, 91
	ldff(40), a
lprintff80_limbo:
	jr lprintff80_limbo

.text@7400
lwaitly_b:
	ld c, 44
lwaitly_b_loop:
	ldff a, (c)
	cmp a, b
	jrnz lwaitly_b_loop
	ret

.data@7a00
	00 00 7f 7f 41 41 41 41
	41 41 41 41 41 41 7f 7f
	00 00 08 08 08 08 08 08
	08 08 08 08 08 08 08 08
	00 00 7f 7f 01 01 01 01
	7f 7f 40 40 40 40 7f 7f
	00 00 7f 7f 01 01 01 01
	3f 3f 01 01 01 01 7f 7f
	00 00 41 41 41 41 41 41
	7f 7f 01 01 01 01 01 01
	00 00 7f 7f 40 40 40 40
	7e 7e 01 01 01 01 7e 7e
	00 00 7f 7f 40 40 40 40
	7f 7f 41 41 41 41 7f 7f
	00 00 7f 7f 01 01 02 02
	04 04 08 08 10 10 10 10
	00 00 3e 3e 41 41 41 41
	3e 3e 41 41 41 41 3e 3e
	00 00 7f 7f 41 41 41 41
	7f 7f 01 01 01 01 7f 7f

