.size 8000

.text@100
	jp lbegin

.data@143
	80

.text@150
lbegin:
	xor a, a
	ldff(04), a
	ldff(24), a
	ldff(25), a
	ldff(26), a
	ld a, 80
	ldff(26), a
	ldff(1a), a
	ld a, fd
	ldff(1b), a
	ld a, f0
	ldff(1c), a
	ld a, 00
	ldff(1d), a
	ld a, c7
	ldff(1e), a

.text@547
	ldff(04), a
	ld b, f8
	ld c, 08
lwait:
	dec b
	jrnz lwait
	dec c
	jrnz lwait
	nop
	nop
	ldff a, (26)
	jp lprint_a

.text@7000
lprint_a:
	push af
	ld b, 91
	call lwaitly_b
	xor a, a
	ldff(40), a
	ld bc, 7a00
	ld hl, 8000
	ld d, 00
lprint_copytiles:
	ld a, (bc)
	inc bc
	ld(hl++), a
	dec d
	jrnz lprint_copytiles
	pop af
	ld b, a
	swap a
	and a, 0f
	ld(9800), a
	ld a, b
	and a, 0f
	ld(9801), a
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
	00 00 08 08 22 22 41 41
	7f 7f 41 41 41 41 41 41
	00 00 7e 7e 41 41 41 41
	7e 7e 41 41 41 41 7e 7e
	00 00 3e 3e 41 41 40 40
	40 40 40 40 41 41 3e 3e
	00 00 7e 7e 41 41 41 41
	41 41 41 41 41 41 7e 7e
	00 00 7f 7f 40 40 40 40
	7f 7f 40 40 40 40 7f 7f
	00 00 7f 7f 40 40 40 40
	7f 7f 40 40 40 40 40 40

