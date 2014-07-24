.size 8000

.text@100
	jp lbegin

.data@143
	80

.text@150
lbegin:
	xor a, a
	ldff(25), a
	ld a, 80
	ldff(16), a
	ldff(17), a
	ld a, c0
	ldff(18), a
	ld a, 07
	ldff(19), a
	ld b, 00
lwaitinittick:
	nop
	nop
	nop
	nop
	dec b
	jrnz lwaitinittick
	ld b, 01
lwaitpos:
	dec b
	jrnz lwaitpos
	ld a, 77
	ldff(24), a
	ld a, 22
	ldff(25), a
	ld a, 80
lmodulate:
	xor a, 40
	ldff(17), a
	ld b, a
	ld a, 80
	ldff(19), a
	ld a, b
	ld b, 20
lwaitperiod:
	dec b
	jrnz lwaitperiod
	jr lmodulate

