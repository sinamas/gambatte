.size 8000

.data@0
	01 

.code@48
	jp ff81

.data@9c
	02 03 04 05 

.code@100
	jp l150

.data@143
	80 00 00 00 1a 00 03 

.code@150
l150:
	ld de, 1028
	ld hl, ffa8
l156:
	dec e
	ld a, (de)
	ld(hl--), a
	jrnz l156
	nop
	nop
	nop
	nop
	nop
	ld c, 44
	ld b, 90
l164:
	ldff a, (c)
	cmp a, b
	jrnz l164
	ld a, 0a
	ld(0000), a
	ld hl, fea0
	ld a, a0
l172:
	dec l
	ld(hl), a
	jrnz l172
	ld c, 44
	ld b, 90
l17a:
	ldff a, (c)
	cmp a, b
	jrnz l17a
	ld hl, c09f
	ld c, a0
	ld a, 10
l185:
	dec c
	ld(hl--), a
	jrnz l185
	ld a, 10
	ld(fe00), a
	ld a, 02
	ldff(45), a
	ld a, 40
	ldff(41), a
	xor a, a
	ldff(0f), a
	ld a, 02
	ldff(ff), a
	ei
	ld a, 93
	ldff(40), a

.code@1000
	ld c, 18
l1002:
	dec c
	jrnz l1002
	nop
	nop
	ld a, c0
	ldff(46), a
	ld c, 0f
l100d:
	dec c
	jrnz l100d
	nop
	nop
	ldff a, (41)
	ld b, 03
	and a, b
	ldff(80), a
	ld c, 28
l101b:
	dec c
	jrnz l101b
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

