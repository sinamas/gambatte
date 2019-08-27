.size 8000

.text@100
	jp lbegin

.data@143
	c0

.text@150
lbegin:
	xor a, a
	ldff(26), a
	ld a, 80
	ldff(26), a
	ld a, 77
	ldff(24), a
	ld a, 00
	ldff(11), a
	ld a, 80
	ldff(12), a
	ld a, e0
	ldff(13), a
	ld a, 87
	ldff(14), a
	ld a, 30
	ldff(00), a
	xor a, a
	ldff(ff), a
	inc a
	ldff(4d), a
	stop, 00
	ldff(4d), a
	stop, 00
	ldff(4d), a
	stop, 00
	ldff(4d), a
	stop, 00
	ldff(4d), a
	stop, 00
	ld a, 11
	ldff(25), a
	ld b, 5b
lwaitpos:
	dec b
	jrnz lwaitpos
	nop
	ld a, 80
lmodulate:
	xor a, 40
	ldff(12), a
	ld b, a
	ld a, 80
	ldff(14), a
	ld a, b
	ld b, 40
lwaitperiod:
	dec b
	jrnz lwaitperiod
	jr lmodulate

