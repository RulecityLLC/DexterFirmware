
.macro ENABLE_CTC_INT_ASM myreg
lds \myreg, TIMSK1
ori \myreg, (1 << OCIE1A)
sts TIMSK1, \myreg
.endm

.macro NOPx8
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
.endm

.macro NOPx6
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
.endm

.macro NOPx32
	NOPx8
	NOPx8
	NOPx8
	NOPx8
.endm
