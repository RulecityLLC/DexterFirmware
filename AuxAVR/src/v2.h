// make interrupt occur on falling edge only
#define SETUP_VSYNC_INT() 	EICRA |= (1 << ISC01); EICRA &= ~(1 << ISC00)

// vsync pin is D2 (INT0)
// csync pin is D3 (INT1)
// field pin is D4
// test pin is B1
#define VSYNC_PIN ((PIND >> 2) & 1)
#define CSYNC_PIN ((PIND >> 3) & 1)
#define FIELD_PIN ((PIND >> 4) & 1)
#define TEST_PIN ((PINB >> 1) & 1)

// set MUX to input 0 (err, I think)
#define ENABLE_WHITE() PORTB &= ~(1 << PB4)

// set MUX to input 1
#define DISABLE_WHITE() PORTB |= (1 << PB4)

// controls video signal muting
#define ENABLE_VIDEO_MUTE() PORTB &= ~(1 << PB3)
#define DISABLE_VIDEO_MUTE() PORTB |= (1 << PB3)

// 10: 0.95V
// 12: 1.13V
// 13: 1.2V

// put 4 output pins (PC0-PC3) into output mode
#define SETUP_WHITE() DDRC = (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3); \
	/* set pins to decimal value of 13 to get about 1.2V (value of 15 is 1.45V) */ \
	PORTC = (1 << PC3) | (1 << PC2) | (0 << PC1) | (1 << PC0)

#define SETUP_CONTROL() DDRB |= (1 << PB3) | (1 << PB4); \
	DISABLE_WHITE(); \
	DISABLE_VIDEO_MUTE()

#define FLASH_SIZE_BYTES 32768

// CRC-16 value stored as the last 2 bytes of application memory.  The bootloader takes up the last 2k, so subtract that first.
#define FLASH_CRC_OFFSET ((FLASH_SIZE_BYTES - 2048) - 2)
