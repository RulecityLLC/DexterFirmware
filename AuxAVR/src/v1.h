
// vsync pin is D2 (INT0)
// csync pin is D3 (INT1)
// field pin is D4
// test pin is B2
#define VSYNC_PIN ((PIND >> 2) & 1)
#define CSYNC_PIN ((PIND >> 3) & 1)
#define FIELD_PIN ((PIND >> 4) & 1)
#define TEST_PIN ((PINB >> 2) & 1)

#define ENABLE_WHITE() PORTC |= (1 << PC2)
#define DISABLE_WHITE() PORTC &= ~(1 << PC2)

// put mute, sync, and white pins in output mode
#define SETUP_WHITE() DDRC = (1 << PC0) | (1 << PC1) | (1 << PC2); \
	// disable MUTE (active low), set SYNC and WHITE to 0
	PORTC |= (1 << PC0); PORTC &= ~(1 << PC1); PORTC &= ~(1 << PC2)
