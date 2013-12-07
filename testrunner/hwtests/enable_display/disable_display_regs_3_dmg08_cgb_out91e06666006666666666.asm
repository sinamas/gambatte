.size 8000

.text@100
	jp lbegin

.data@143
	80

.text@150
lbegin:
	ld b, 92
	ld c, 44
lbegin_waitly92:
	ldff a, (c)
	cmp a, b
	jrnz lbegin_waitly92
	ld a, 00
	ldff(40), a
	ld a, 66
	ldff(40), a
	ldff(41), a
	ldff(42), a
	ldff(43), a
	ldff(44), a
	ldff(45), a
	ldff(47), a
	ldff(48), a
	ldff(49), a
	ldff(4a), a
	ldff(4b), a
	ld a, 91
	ldff(40), a
	ldff a, (40)
	ldff(80), a
	ldff a, (41)
	ldff(81), a
	ldff a, (42)
	ldff(82), a
	ldff a, (43)
	ldff(83), a
	ldff a, (44)
	ldff(84), a
	ldff a, (45)
	ldff(85), a
	ldff a, (47)
	ldff(86), a
	ldff a, (48)
	ldff(87), a
	ldff a, (4a)
	ldff(88), a
	ldff a, (4b)
	ldff(89), a
	jp lprint_hiram

.text@7000
lprint_hiram:
	ld c, 44
	ld b, 91
lprint_waitly91:
	ldff a, (c)
	cmp a, b
	jrnz lprint_waitly91
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
	ld hl, 9800
	ld c, 80
	ld b, 0a
lprint_set_bgmap_from_hiram:
	ldff a, (c)
	rrca
	rrca
	rrca
	rrca
	and a, 0f
	ld(hl++), a
	ldff a, (c)
	and a, 0f
	ld(hl++), a
	inc c
	dec b
	jrnz lprint_set_bgmap_from_hiram
	xor a, a
	ldff(42), a
	ldff(43), a
	ld a, 91
	ldff(40), a
lprint_limbo:
	jr lprint_limbo

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

