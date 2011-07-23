.size 8000

.code@49
	jp l1000

.code@100
	jp l150

.data@143
	c0 

.code@150
l150:
	ld a, 00
	ldff(ff), a
	ld a, 30
	ldff(00), a
	ld a, 01
	ldff(4d), a
	stop, 00
	ld c, 44
	ld b, 90
l162:
	ldff a, (c)
	cmp a, b
	jpnz l162
	ld a, 11
	ldff(40), a
	ld hl, 8000
	ld b, 08
l170:
	ld a, 00
	ld(hl++), a
	ld a, 7e
	ld(hl++), a
	dec b
	jrnz l170
	ld b, 08
l17b:
	ld a, 00
	ld(hl++), a
	ld a, 81
	ld(hl++), a
	dec b
	jrnz l17b
	ld b, 08
l186:
	ld a, ff
	ld(hl++), a
	ld a, 81
	ld(hl++), a
	dec b
	jrnz l186
	ld b, 08
l191:
	ld a, ff
	ld(hl++), a
	ld a, 7e
	ld(hl++), a
	dec b
	jrnz l191
	ld c, 12
	ld hl, 9800
l19f:
	ld b, 06
	ld a, 02
l1a3:
	ld(hl++), a
	inc a
	ld(hl++), a
	dec a
	dec b
	jrnz l1a3
	ld b, 0a
l1ac:
	xor a, a
	ld(hl++), a
	inc a
	ld(hl++), a
	dec b
	jrnz l1ac
	dec c
	jrnz l19f
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

.code@1000
l1000:
	ldff(c), a
	ld a, 04

.code@1020
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

.code@108d
	ldff a, (41)
	and a, 07
	ldff(80), a
	jp l7000

.code@2000
l2000:
	nop

.code@6ffd
	jp l2000
l7000:
	ld c, 44
	ld b, 91
l7004:
	ldff a, (c)
	cmp a, b
	jpnz l7004
	xor a, a
	ldff(40), a
	ld bc, 7a00
	ld hl, 8000
	ld d, a0
l7014:
	ld a, (bc)
	inc bc
	ld(hl++), a
	dec d
	jpnz l7014
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
	xor a, a
	ldff(43), a
	ld a, 91
	ldff(40), a
	jp l2000

.data@7a02
	7f 7f 41 41 41 41 41 41 
	41 41 41 41 7f 7f 00 00 
	08 08 08 08 08 08 08 08 
	08 08 08 08 08 08 00 00 
	7f 7f 01 01 01 01 7f 7f 
	40 40 40 40 7f 7f 00 00 
	7f 7f 01 01 01 01 3f 3f 
	01 01 01 01 7f 7f 00 00 
	41 41 41 41 41 41 7f 7f 
	01 01 01 01 01 01 00 00 
	7f 7f 40 40 40 40 7e 7e 
	01 01 01 01 7e 7e 00 00 
	7f 7f 40 40 40 40 7f 7f 
	41 41 41 41 7f 7f 00 00 
	7f 7f 01 01 02 02 04 04 
	08 08 10 10 10 10 00 00 
	3e 3e 41 41 41 41 3e 3e 
	41 41 41 41 3e 3e 00 00 
	7f 7f 41 41 41 41 7f 7f 
	01 01 01 01 7f 7f 

