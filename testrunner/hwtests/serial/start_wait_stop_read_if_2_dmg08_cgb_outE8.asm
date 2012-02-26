.size 8000

.code@58
	jp lstartserial

.code@100
	jp l150

.data@143
	80

.code@150
l150:
	xor a, a
	ldff(0f), a
	ld a, 08
	ldff(ff), a
	ld a, 81
	ldff(02), a
	ei

.code@1000
lstartserial:
	xor a, a
	ldff(01), a
	ld a, 81
	ldff(02), a

.code@13f1
	ld a, 01
	ldff(02), a

.code@1410
	ldff a, (0f)
	ld b, a
	srl a
	srl a
	srl a
	srl a
	ldff(80), a
	ld a, b
	and a, 0f
	ldff(81), a
	jp lprintff80

.code@7000
lprintff80:
	ld c, 44
	ld b, 91
lwaitvblank:
	ldff a, (c)
	cmp a, b
	jpnz lwaitvblank
	xor a, a
	ldff(40), a
	ld bc, 7a00
	ld hl, 8000
	ld d, 00
lcopytiles:
	ld a, (bc)
	inc bc
	ld(hl++), a
	dec d
	jpnz lcopytiles
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
	ld a, (ff80)
	ld(9800), a
	ld a, (ff81)
	ld(9801), a
	xor a, a
	ldff(43), a
	ld a, 91
	ldff(40), a
linf:
	jr linf

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

