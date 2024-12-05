
.macro ENABLE_CTC_INT_ASM myreg
lds \myreg, TIMSK1
ori \myreg, (1 << OCIE1A)
sts TIMSK1, \myreg
.endm

.macro DISABLE_CTC_INT_ASM myreg
lds \myreg, TIMSK1
andi \myreg, ~(1 << OCIE1A)
sts TIMSK1, \myreg
.endm

; [0xFD is ~(1 << OCIE1A)]

.macro DISABLE_VSYNC_INT_ASM
cbi	_SFR_IO_ADDR(EIMSK),VSYNC_EXT_INT
.endm

.macro DISABLE_INT0
cbi	_SFR_IO_ADDR(EIMSK),INT0
.endm

.macro ENABLE_INT0
sbi	_SFR_IO_ADDR(EIMSK),INT0
.endm

.macro DISABLE_INT1
cbi	_SFR_IO_ADDR(EIMSK),INT1
.endm

.macro ENABLE_INT1
sbi	_SFR_IO_ADDR(EIMSK),INT1
.endm
