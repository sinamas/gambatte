.size 8000

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
	nop
	nop
	nop
	nop
	nop
	nop
	ld c, 44
	ld b, 91
l162:
	ldff a, (c)
	cmp a, b
	jpnz l162
	xor a, a
	ldff(40), a
	ld a, 80
	ldff(68), a
	xor a, a
	ldff(69), a
	ldff(69), a
	ld a, 86
	ldff(68), a
	ld a, ff
	ldff(69), a
	ldff(69), a
	ld hl, 8000
	xor a, a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	dec a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld(hl++), a
	ld hl, 9800
	ld a, 00
	ld b, 03
l1a9:
	ld c, 00
l1ab:
	ld(hl++), a
	dec c
	jrnz l1ab
	dec b
	jrnz l1a9
	ld hl, 9c00
	ld a, 01
	ld b, 03
l1b9:
	ld c, 00
l1bb:
	ld(hl++), a
	dec c
	jrnz l1bb
	dec b
	jrnz l1b9
	ld hl, 9c00
	ld a, 00
	ld de, 0021
	ld b, 20
l1cc:
	ld(hl), a
	add hl, de
	dec b
	jrnz l1cc
	ld a, 03
	ldff(47), a
	ld a, 01
	ldff(4a), a
	ld a, a6
	ldff(4b), a
	ld a, f1
	ldff(40), a
	ld c, 44
l1e3:
	ld b, 02
l1e5:
	ldff a, (c)
	cmp a, b
	jrnz l1e5
	ld a, a5
	ldff(4b), a
	ld b, 90
l1ef:
	ldff a, (c)
	cmp a, b
	jrnz l1ef
	ld a, a6
	ldff(4b), a
	jr l1e3

.code@2000
l2000:
	nop

.code@6ffd
	jp l2000
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

