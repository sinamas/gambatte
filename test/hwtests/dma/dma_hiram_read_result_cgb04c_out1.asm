.size 8000

.text@100
	jp lbegin

.data@143
	c0

.text@150
lbegin:
	ld b, 90
	call lwaitly_b
	ld hl, ff80
	ld b, 08
	xor a, a
lbegin_inithiram:
	inc a
	ld(hl++), a
	dec b
	jrnz lbegin_inithiram
	ld a, ff
	ldff(51), a
	ld a, 80
	ldff(52), a
	ld a, 80
	ldff(53), a
	xor a, a
	ldff(54), a
	ldff(55), a
	ld a, (8007)
	sub a, fe
	jp lprint_a

.text@7000
lprint_a:
	push af
	ld b, 91
	call lwaitly_b
	xor a, a
	ldff(40), a
	pop af
	ld(9800), a
	ld bc, 7a00
	ld hl, 8000
	ld d, a0
lprint_copytiles:
	ld a, (bc)
	inc bc
	ld(hl++), a
	dec d
	jrnz lprint_copytiles
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
	xor a, a
	ldff(69), a
	ldff(69), a
	ldff(43), a
	ld a, 91
	ldff(40), a
lprint_limbo:
	jr lprint_limbo

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

