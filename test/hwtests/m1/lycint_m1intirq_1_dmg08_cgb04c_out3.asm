.size 8000

.text@48
	dec d
	jrnz l4c
	ret
l4c:
	jp l1000

.text@100
	jp lbegin

.data@143
	80

.text@150
lbegin:
	ld a, ff
	ldff(45), a
	ld b, 8d
	call lwaitly_b
	ld a, 50
	ldff(41), a
	ld a, 02
	ldff(ff), a
	ei
	ld a, b
	inc a
	inc a
	ldff(45), a
	ld c, 0f
	xor a, a
	ldff(0f), a
	ld b, 03
	ld d, 02

.text@1000
l1000:
	ei

.text@1064
	ldff a, (c)
	and a, b
	ld(ff80), a
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

