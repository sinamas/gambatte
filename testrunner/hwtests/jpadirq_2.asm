.size 8000

.text@60
	pop af
	ldff a, (80)
	inc a
	ldff(80), a
	ei
	jp lprint

.text@100
	jp lbegin

.data@143
	80

.text@150
lbegin:
	ld b, 90
	call lwaitly_b
	xor a, a
	ldff(40), a
	ld bc, 7a00
	ld hl, 8000
	ld d, 00
lbegin_copytiles:
	ld a, (bc)
	inc bc
	ld(hl++), a
	dec d
	jrnz lbegin_copytiles
	xor a, a
	ld(9800), a
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
	ld a, 10
	ldff(00), a
	ldff(ff), a
	xor a, a
	ldff(0f), a
	ldff(80), a
	ldff(81), a
	ei
	jp lprint

.text@7000
lprint:
	ld b, 90
lprint_waitly90:
	ldff a, (44)
	cmp a, b
	jrnz lprint_waitly90
	ldff a, (80)
	rrca
	rrca
	rrca
	rrca
	and a, 0f
	ld(9800), a
	ldff a, (80)
	and a, 0f
	ld(9801), a
	ldff a, (00)
	ld b, a
	rrca
	rrca
	rrca
	rrca
	and a, 0f
	ld(9802), a
	ld a, b
	and a, 0f
	ld(9803), a
	ldff a, (81)
	inc a
	ldff(81), a
	jrnz lprint
	ld a, b
	xor a, 30
	ldff(00), a
	jr lprint

.text@7400
lwaitly_b:
	ldff a, (44)
	cmp a, b
	jrnz lwaitly_b
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

