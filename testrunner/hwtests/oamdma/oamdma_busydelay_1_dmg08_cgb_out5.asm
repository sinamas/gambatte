.size 8000

.code@0
l0:
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
	nop
	nop
	jp l1800

.code@100
	jp l150

.data@143
	c0 

.code@150
l150:
	ld c, 44
	ld b, 90
l154:
	ldff a, (c)
	cmp a, b
	jrnz l154
	ld a, 04
	ldff(46), a
	inc a
	inc a
	inc a
	inc a
	inc a
	jp l1800

.code@404
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0
	jp l0

.code@1800
l1800:
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

