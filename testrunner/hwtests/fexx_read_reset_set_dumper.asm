.size 8000

.code@50
	jp l1000

.code@100
	jp l150

.data@143
	80 00 00 00 03 00 01 

.code@150
l150:
	ldff a, (44)
	cmp a, 91
	jrnz l150
	ldff a, (40)
	xor a, 80
	ldff(40), a
	nop
	nop
	nop
	nop
	jp l1000

.code@1000
l1000:
	ld a, 0a
	ld(0000), a
	ld hl, a000
	call l1800
	nop
	nop
	nop
	ld de, fe00
	ld b, 00
	xor a, a
l1014:
	ld(de), a
	inc e
	dec b
	jrnz l1014
	call l1800
	nop
	nop
	nop
	ld de, fe00
	ld b, 00
	ld a, ff
l1026:
	ld(de), a
	inc e
	dec b
	jrnz l1026
	call l1800
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	xor a, a
	ld(0000), a
	ldff a, (40)
	xor a, 80
	ldff(40), a
	jp l7000

.code@1800
l1800:
	ld de, fe00
	ld b, 00
l1805:
	ld a, (de)
	inc e
	ld(hl++), a
	dec b
	jrnz l1805
	ret

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
	ld d, 00
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
	ld a, (ff81)
	ld(9801), a
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
	01 01 01 01 7f 7f 00 00 
	08 08 22 22 41 41 7f 7f 
	41 41 41 41 41 41 00 00 
	7e 7e 41 41 41 41 7e 7e 
	41 41 41 41 7e 7e 00 00 
	3e 3e 41 41 40 40 40 40 
	40 40 41 41 3e 3e 00 00 
	7e 7e 41 41 41 41 41 41 
	41 41 41 41 7e 7e 00 00 
	7f 7f 40 40 40 40 7f 7f 
	40 40 40 40 7f 7f 00 00 
	7f 7f 40 40 40 40 7f 7f 
	40 40 40 40 40 40 

