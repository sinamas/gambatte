.size 8000

.code@48
	jp l1000

.code@100
	jp l150

.data@143
	c0 

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
	jpnz l162
	ld hl, fe00
	ld b, 08
	xor a, a
l16d:
	inc a
	ld(hl++), a
	dec b
	jrnz l16d
	ld a, fe
	ldff(51), a
	ld a, 00
	ldff(52), a
	ld a, 80
	ldff(53), a
	xor a, a
	ldff(54), a
	ldff(55), a
	ld hl, fe07
	ld de, 8007
l189:
	ld a, (de)
	ld b, a
	ld a, (hl--)
	cmp a, b
	jrnz l192
	dec e
	jrnz l189
l192:
	ld a, e
	ld(ff80), a
	jp l7000

.code@1000
l1000:
	ld a, ff
	ldff(53), a
	ld a, ff
	ldff(54), a
	ld a, bf
	ldff(51), a
	ld a, f0
	ldff(52), a
	ld b, 07
	ld a, 01
	ldff(55), a
	ld a, (hl)
	and a, b
	ld(ff80), a
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

