.size 8000

.text@100
	jp lbegin

.data@143
	80

.text@150
lbegin:
	xor a, a
	ldff(24), a
	ldff(25), a
	ldff(26), a
	ld a, 80
	ldff(26), a
	ld b, 05
	ld c, 07
lwaitbegin:
	dec b
	jrnz lwaitbegin
	dec c
	jrnz lwaitbegin
	nop
	nop
	nop
	nop
	ld a, 20
	ldff(10), a
	ld a, f0
	ldff(12), a
	ld a, 00
	ldff(13), a
	ld a, 87
	ldff(14), a
	ld b, f2
	ld c, 10
lwait:
	dec b
	jrnz lwait
	dec c
	jrnz lwait
	nop
	nop
	nop
	nop
	xor a, a
	ldff(10), a
	ld a, 77
	ldff(24), a
	ld a, 11
	ldff(25), a
limbo:
	jr limbo

