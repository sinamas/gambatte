.size 8000

.text@100
	jp lbegin

.data@143
	80 00 00 00 1b 00 03

.text@e00
lcopy_de_to_hl_until_de_is_a0xx:
	ld b, 00
lcopy_de_to_hl_until_de_is_a0xx_inner:
	ld a, (de)
	inc e
	ld(hl++), a
	dec b
	jrnz lcopy_de_to_hl_until_de_is_a0xx_inner
	inc d
	ld a, a0
	cmp a, d
	jrnz lcopy_de_to_hl_until_de_is_a0xx
	ret

.text@1000
lbegin:
	ldff a, (44)
	cmp a, 91
	jrnz lbegin
	xor a, a
	ldff(40), a
	ld a, 0a
	ld(0000), a
	xor a, a
	ld(4000), a
	ldff(4f), a
	ld hl, a000
	ld de, 8000
	call lcopy_de_to_hl_until_de_is_a0xx
	ld a, 01
	ld(4000), a
	ldff(4f), a
	ld hl, a000
	ld de, 8000
	call lcopy_de_to_hl_until_de_is_a0xx
	xor a, a
	ld(0000), a
	ld a, 81
	ldff(40), a
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
	srl a
	srl a
	srl a
	srl a
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

