.size 8000

.text@100
	jp lbegin

.data@143
	c0

.text@150
lbegin:
	ld a, ff
	ldff(45), a
	ld c, 44
	ld b, 91
lbegin_waitly91:
	ldff a, (c)
	cmp a, b
	jpnz lbegin_waitly91
	ld hl, fe00
	ld d, 10
	ld a, d
	ld(hl++), a
	ld a, a8
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, a8
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, a8
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, a8
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, a8
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, a8
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, a8
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, a8
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, a8
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, a8
	ld(hl++), a
	inc l
	inc l
	ld a, d
	ld(hl++), a
	ld a, 54
	ld(hl++), a
	ld a, 01
	ld(hl++), a
	ld hl, 8010
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
	ld a, 93
	ldff(40), a
	ld a, 80
	ldff(6a), a
	xor a, a
	ld c, 6b
	ldff(c), a
	ldff(c), a
	ldff(c), a
lbegin_limbo:
	jr lbegin_limbo

