.size 8000

.data@0
	01 

.code@48
	jp l1000

.data@9c
	02 03 04 05 

.code@100
	jp l150

.data@143
	80 00 00 00 1b 00 03 

.code@150
l150:
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
	nop
	ld c, 44
	ld b, 90
l162:
	ldff a, (c)
	cmp a, b
	jrnz l162
	ld a, 0a
	ld(0000), a
	ld hl, fe00
	ld c, a0
	ld a, a0
l172:
	ld(hl), a
	inc l
	dec c
	jrnz l172
	ld c, 44
	ld b, 90
l17b:
	ldff a, (c)
	cmp a, b
	jrnz l17b
	ld hl, 809f
	ld a, a0
l184:
	dec a
	ld(hl--), a
	jrnz l184
	ld a, 02
	ldff(45), a
	ld a, 40
	ldff(41), a
	xor a, a
	ldff(0f), a
	ld a, 02
	ldff(ff), a
	ei

.code@1000
l1000:
	ld a, 80
	ldff(46), a
	ld c, 44
	ld b, 91
l1008:
	ldff a, (c)
	cmp a, b
	jrnz l1008
	xor a, a
	ldff(40), a
	ld hl, fe9f
	ld de, a0a0
l1015:
	ld a, (hl)
	dec l
	dec e
	ld(de), a
	jrnz l1015
	xor a, a
	ld(0000), a

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

