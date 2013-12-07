.size 8000

.text@49
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
	ld b, 90
	call lwaitly_b
	ld a, 11
	ldff(40), a
	ld hl, 8000
	ld b, 08
lbegin_settile0data:
	ld a, 00
	ld(hl++), a
	ld a, 7e
	ld(hl++), a
	dec b
	jrnz lbegin_settile0data
	ld b, 08
lbegin_settile1data:
	ld a, 00
	ld(hl++), a
	ld a, 81
	ld(hl++), a
	dec b
	jrnz lbegin_settile1data
	ld b, 08
lbegin_settile2data:
	ld a, ff
	ld(hl++), a
	ld a, 81
	ld(hl++), a
	dec b
	jrnz lbegin_settile2data
	ld b, 08
lbegin_settile3data:
	ld a, ff
	ld(hl++), a
	ld a, 7e
	ld(hl++), a
	dec b
	jrnz lbegin_settile3data
	ld c, 12
	ld hl, 9800
lbegin_set_bgmap:
	ld b, 06
	ld a, 02
lbegin_set_bgmapline_tilenos0to11:
	ld(hl++), a
	inc a
	ld(hl++), a
	dec a
	dec b
	jrnz lbegin_set_bgmapline_tilenos0to11
	ld b, 0a
lbegin_set_bgmapline_tilenos12to31:
	xor a, a
	ld(hl++), a
	inc a
	ld(hl++), a
	dec b
	jrnz lbegin_set_bgmapline_tilenos12to31
	dec c
	jrnz lbegin_set_bgmap
	ld a, e4
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
	ld a, 20
	ldff(41), a
	ld a, 02
	ldff(ff), a
	ld c, 43
	ld a, 91
	ldff(40), a
	ei
	ld a, 07

.text@1000
lstatint:
	ldff(c), a
	ld a, 04

.text@1020
	ld b, 02
	ldff(c), a
	sub a, b
	ldff(c), a
	sub a, b
	ldff(c), a
	sub a, b
	ldff(c), a
	sub a, b
	ldff(c), a
	sub a, b
	ldff(c), a
	sub a, b
	ldff(c), a
	sub a, b
	ldff(c), a
	sub a, b
	ldff(c), a
	sub a, b
	ldff(c), a
	sub a, b
	ldff(c), a
	sub a, b
	ldff(c), a

.text@108c
	ldff a, (41)
	and a, 07
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

