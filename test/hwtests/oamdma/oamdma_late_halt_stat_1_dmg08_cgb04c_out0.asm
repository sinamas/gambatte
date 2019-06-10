.size 8000

.text@50
	jp ltimaint

.text@100
	jp lbegin

.data@143
	80

.text@150
lbegin:
	ld b, 91
	call lwaitly_b
	xor a, a
	ldff(40), a
	ld hl, c000
	ld b, a0
lbegin_fill_wram:
	ld(hl++), a
	dec b
	jrnz lbegin_fill_wram
	ld a, 10
	ld(fe00), a
	ld(fe01), a
	ld(c000), a
	ld(c001), a
	ld hl, 0800
	ld c, 80
	ld b, 20
lbegin_copy_dma_routine:
	ld a, (hl++)
	ldff(c), a
	inc c
	dec b
	jrnz lbegin_copy_dma_routine
	ld a, fe
	ldff(05), a
	ldff(06), a
	ld a, 04
	ldff(07), a
	ld a, 06
	ldff(ff), a
	xor a, a
	ldff(0f), a
	ei

.text@800
	ld a, c0
	ldff(46), a
	ld b, 26
lwait_oam_dma:
	dec b
	jrnz lwait_oam_dma
	nop
	xor a, a
	ldff(0f), a
	halt
	nop
	jp loam_dma_done

.text@1000
ltimaint:

.text@1028
	ld a, 93
	ldff(40), a
	jp ff80
loam_dma_done:

.text@1063
	ldff a, (41)
	jp lprint_a

.text@7000
lprint_a:
	and a, 07
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

