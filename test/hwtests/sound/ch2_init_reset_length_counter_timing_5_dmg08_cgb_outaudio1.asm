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
	ld b, 07
	ld c, 05
lwaitbegin:
	dec b
	jrnz lwaitbegin
	dec c
	jrnz lwaitbegin
	nop
	nop
	nop
	ld a, 3d
	ldff(16), a
	ld a, f0
	ldff(17), a
	ld a, 00
	ldff(18), a
	ld a, c7
	ldff(19), a
	ld b, f9
	ld c, 06
lwait:
	dec b
	jrnz lwait
	dec c
	jrnz lwait
	nop
	nop
	ld a, 07
	ldff(19), a
	ld a, 77
	ldff(24), a
	ld a, 22
	ldff(25), a
limbo:
	jr limbo

