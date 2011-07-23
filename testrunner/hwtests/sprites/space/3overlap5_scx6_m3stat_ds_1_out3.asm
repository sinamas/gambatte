.size 8000

.code@48
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
	xor a, a
	dec a
	ldff(45), a
	ld c, 44
	ld b, 91
l166:
	ldff a, (c)
	cmp a, b
	jpnz l166
	ld hl, fe00
	ld d, 10
	ld a, d
	ld(hl++), a
	ld a, 08
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, 0b
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, 0e
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, 28
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, 2b
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, 2e
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, 48
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, 4b
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, 4e
	ld(hl++), a
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	ld a, 40
	ldff(41), a
	ld a, 02
	ldff(ff), a
	xor a, a
	ldff(0f), a
	ei
	ld a, 01
	ldff(45), a
	ld c, 41
	ld b, 03
	ld a, 93
	ldff(40), a
	ld a, 06
	ldff(43), a

.code@1000
l1000:
	nop

.code@1097
	ldff a, (c)
	and a, b
	ld(ff80), a
	xor a, a
	ldff(43), a
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

