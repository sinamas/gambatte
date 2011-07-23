.size 8000

.code@50
	jp l1000

.code@100
	jp l150

.data@143
	80 00 00 00 1b 00 03 

.code@150
l150:
	jp l1000

.code@e00
le00:
	ld b, 00
le02:
	ld a, (de)
	inc e
	ld(hl++), a
	dec b
	jrnz le02
	inc d
	ld a, e0
	cmp a, d
	jrnz le00
	ret

.code@1000
l1000:
	ldff a, (44)
	cmp a, 91
	jrnz l1000
	xor a, a
	ldff(40), a
	ld a, 0a
	ld(0000), a
	xor a, a
	ld(4000), a
	ldff(70), a
	ld hl, a000
	ld de, c000
	call le00
	ld a, 01
	ld(4000), a
	ld a, 02
	ldff(70), a
	ld hl, a000
	ld de, d000
	call le00
	ld a, 03
	ldff(70), a
	ld de, d000
	call le00
	ld a, 02
	ld(4000), a
	ld a, 04
	ldff(70), a
	ld hl, a000
	ld de, d000
	call le00
	ld a, 05
	ldff(70), a
	ld de, d000
	call le00
	ld a, 03
	ld(4000), a
	ld a, 06
	ldff(70), a
	ld hl, a000
	ld de, d000
	call le00
	ld a, 07
	ldff(70), a
	ld de, d000
	call le00
	xor a, a
	ld(0000), a
	ld a, 81
	ldff(40), a
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

