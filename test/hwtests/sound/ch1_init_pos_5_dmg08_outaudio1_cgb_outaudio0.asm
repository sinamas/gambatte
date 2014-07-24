.size 8000

.text@100
	jp lbegin

.data@143
	80

.text@150
lbegin:
	xor a, a
	ldff(25), a
	ldff(10), a
	ld a, 80
	ldff(11), a
	ldff(12), a
	ld b, 55
lwaitpos:
	dec b
	jrnz lwaitpos
	nop
	ld a, 77
	ldff(24), a
	ld a, 11
	ldff(25), a
	ld a, 80
lmodulate:
	xor a, 40
	ldff(12), a
	ld b, a
	ld a, 80
	ldff(14), a
	ld a, b
	ld b, 20
lwaitperiod:
	dec b
	jrnz lwaitperiod
	jr lmodulate

