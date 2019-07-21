.size 8000

.data@00
	9f 9e 9d 9c 9b 9a 99 98 97 96 95 94 93 92 91 90

.text@50
	jp ltimaint

.text@100
	jp lbegin

.data@143
	c0

.text@150
lbegin:
	ld hl, c000
	ld c, a0
	ld a, 50
lbegin_fill_wram:
	ld(hl++), a
	inc a
	dec c
	jrnz lbegin_fill_wram
	ld b, 91
	call lwaitly_b
	xor a, a
	ldff(40), a
	ld hl, fe00
	ld c, a0
	ld a, f0
lbegin_fill_oam:
	ld(hl++), a
	dec c
	jrnz lbegin_fill_oam
	ld a, fe
	ldff(05), a
	ldff(06), a
	ld a, 04
	ldff(07), a
	ld a, 04
	ldff(ff), a
	xor a, a
	ldff(0f), a
	ei

.text@1000
ltimaint:

.text@10c0
	ld a, 91
	ldff(40), a
	ld a, 01
	ldff(43), a
	xor a, a
	ldff(51), a
	ldff(52), a
	ldff(53), a
	ldff(54), a

.text@1160
	ld a, c0
	ldff(46), a
	ld a, 80
	ldff(55), a
	xor a, a
	ldff(0f), a
	halt

.text@1480
	ld sp, fe00
	pop bc
	pop de
	ld sp, ffff
	push de
	push bc
	jp lprint4

.text@7000
lprint4:
	ld b, 90
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
	ld hl, 9800
	ld d, 02
lprint_settiles:
	pop bc
	ld a, c
	swap a
	and a, 0f
	ld(hl++), a
	ld a, c
	and a, 0f
	ld(hl++), a
	ld a, b
	swap a
	and a, 0f
	ld(hl++), a
	ld a, b
	and a, 0f
	ld(hl++), a
	dec d
	jrnz lprint_settiles
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

