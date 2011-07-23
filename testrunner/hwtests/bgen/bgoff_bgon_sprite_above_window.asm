.size 8000

.code@48
	pop de
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
	ld c, 69
	xor a, a
	ldff(c), a
	ldff(c), a
	ld a, 86
	ldff(68), a
	ld a, ff
	ldff(c), a
	ldff(c), a
	ld a, 80
	ldff(6a), a
	ld c, 6b
	ld a, 55
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ldff(c), a
	ld hl, 8000
	ld a, aa
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
	ld a, ff
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
	ld a, 55
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
	xor a, a
	ld hl, fea0
l1c8:
	dec l
	ld(hl), a
	jrnz l1c8
	ld a, 10
	ld(hl), a
	inc l
	ld a, 11
	ld(hl), a
	inc l
	ld a, 01
	ld(hl), a
	inc l
	ld a, 00
	ld(hl), a
	inc l
	ld a, 78
	ld(hl), a
	inc l
	ld a, 92
	ld(hl), a
	inc l
	ld a, 01
	ld(hl), a
	inc l
	ld a, 00
	ld(hl), a
	ld hl, 9800
	ld a, 00
	ld b, 03
l1f2:
	ld c, 00
l1f4:
	ld(hl++), a
	dec c
	jrnz l1f4
	dec b
	jrnz l1f2
	ld hl, 9c00
	ld a, 01
	ld b, 03
l202:
	ld c, 00
l204:
	ld(hl++), a
	dec c
	jrnz l204
	dec b
	jrnz l202
	ld hl, 9c00
	ld a, 00
	ld de, 0021
	ld b, 20
l215:
	ld(hl), a
	add hl, de
	dec b
	jrnz l215
	ld hl, 9c10
	ld a, 02
	ld de, 0021
	ld b, 20
l224:
	ld(hl), a
	add hl, de
	dec b
	jrnz l224
	ld a, 03
	ldff(47), a
	ld a, 55
	ldff(48), a
	ld a, 18
	ldff(4a), a
	ld a, 0f
	ldff(4b), a
	ld a, f7
	ldff(40), a
	ld a, 20
	ldff(41), a
	xor a, a
	ldff(0f), a
	ld a, 02
	ldff(ff), a
	ei

.code@1000
l1000:
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	ld a, f6
	ldff(40), a

.code@102d
	ld a, f7
	ldff(40), a
	ei

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

