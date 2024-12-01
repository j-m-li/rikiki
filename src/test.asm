; test.asm

.code32
.org 0
entry:
	add a,a,c
	add b,a,c
	add c,a,c
	add a,a,b
	add b,a,b
	add c,a,b
	add a,a,a
	add b,a,a
	add c,a,a 

	ldi a,-4
	ldi a,0
	ldi a,4

	neg a,a
	div a,a,3
	mul a,a,3

	cmp a,a		; compare
	cmp a,b
	cmp a,c
	cmp b,a
	;cmp b,b -> swp a,b
	cmp b,c
	cmp c,a
	cmp c,b
	;cmp c,c -> swp a,c 
	bgt greater
	blt lesser
	beq equal

memory:
	pge -1		; page selection (only 6 pages available)
	pge 0
	pge 4

	lod a, -4	; load
	lod a,  0
	lod a, +4

	sto a, -4	; store
	sto a,  0

	ldi a,  0 ; prepare high level function 0 (write to program counter)
	sto a, +4 ; trigers high level function

greater:
	ldi a,+1
	swp a,b
	swp a,c
lesser:
	ldi a,-1
equal:
	cmp a,a
	beq entry

.byte 0,127,128,255
.word 1,43046721, equal

.end



