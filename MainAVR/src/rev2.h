// Definitions for dexter rev2 board

// PORTA will be data lines (8 bits of data I/O)
#define DATA_CONTROL DDRA
#define DATA_PORT_WRITE PORTA
#define DATA_PORT_READ PINA

// vsync pin will be B2 (INT2)
// field pin will be B3
#define VSYNC_PIN ((PINB >> 2) & 1)
#define FIELD_PIN ((PINB >> 3) & 1)

#define VSYNC_EXT_INT INT2

// make interrupt occur on falling edge only
#define SETUP_VSYNC_INT() 	EICRA |= (1 << ISC21); EICRA &= ~(1 << ISC20)

// SPI defines
#define DDR_SPI DDRB
#define DD_SS PB4
#define DD_MOSI PB5
#define DD_MISO PB6
#define DD_SCK PB7

#define LED_VSYNC_PIN_TOGGLE() leds_toggle(LED_VIDEO_OK)

#define SETUP_OPTO_RELAY() DDRC |= (1 << PC7)
#define ENABLE_OPTO_RELAY() DDRC |= (1 << PC7); PORTC |= (1 << PC7)
#define DISABLE_OPTO_RELAY() DDRC |= (1 << PC7); PORTC &= ~(1 << PC7)

///////////////////////////////////////////////////////////////

#define SETUP_BUTTONS() \
	DDRD &= ~(1 << PD6);	/* set MODE line to input mode */ \
	PORTD |= (1 << PD6);	/* set pullup resistor so that we just have to see if PIND is ground to know if it's being pressed */ \
	DDRB &= ~(1 << PB6);	/* set DIAGNOSE line to input mode */ \
	PORTB |= (1 << PB6);	/* set pullup resistor */

#define BUTTON_DIAGNOSE (PINB & (1 << PB6))
#define BUTTON_MODE (PIND & (1 << PD6))

///////////////////////////////////////////////////////////////

#define SETUP_SVIDEO_MUTE() \
	DDRD |= (1 << PD7);	/* put PD7 in output mode */ \
	PORTD |= (1 << PD7);	/* set VMUTE hi so video is not muted */ \
	PORTD &= ~(1 << PD7);	// update, mute s-video so that composite works properly

///////////////////////////////////////////////////////////////

// LED mappings

#define	LED_LDV8000 0
#define LED_SIMUTREK 1
#define LED_LDP1450 2
#define LED_LDP1000A 3
#define	LED_VIP9500SG 4
#define LED_OTHER 5
#define	LED_PR8210 6
#define	LED_DIAGNOSE 7
#define	LED_VIDEO_OK 8
#define	LED_LDV1000 9
#define	LED_LDV1000_BL 10
#define	LED_PR7820 11
#define	LED_PR8210A 12
#define	LED_VP380 13
#define	LED_VP932 14
#define	LED_VP931 15

///////////////////////////////////////////////////////////////

#define SETUP_DCE_AND_DTE() DDRB |= ((1 << PB0) | (1 << PB1))

// we leave PC0 alone here because it is a side effect that a driver probably does not want (and it is not necessary since both MAX chips will be disabled anyway)
#define DISABLE_DCE_AND_DTE() PORTB &= ~((1 << PB0) | (1 << PB1))

// PC0 is RTS for DTE mode or CTS for DCE mode and should always be enabled for hardware flow control
// As TTL serial is active low, we set PC0 to 0.
#define ENABLE_DCE() PORTB &= ~(1 << PB0); PORTB |= (1 << PB1); DDRC |= (1 << PC0); PORTC &= ~(1 << PC0)
#define ENABLE_DTE() PORTB &= ~(1 << PB1); PORTB |= (1 << PB0); DDRC |= (1 << PC0); PORTC &= ~(1 << PC0)

// More on TTL serial being active low:
// MAX datasheet says:
//"The inputs of unused drivers can be left unconnected
//since 400kO input pullup resistors to VCC are built in
//(except for the MAX220). The pullup resistors force the
//outputs of unused drivers low because all drivers invert."

///////////////////////////////////////////////////////////////

// Set (PR8210 int/ext', remote control, scan c, and jmptrig') to input mode and raise pull-up resistors for them all
// PC0: Jmp Trigger INT/EXT' (tied to Scan C INT/EXT')
// PC1: Scan C INT/EXT'
// PC6: remote control
// PC7: jump trigger (NOTE : this is also used by opto relay, so this setup should happen after)
// PD4: Scan C (this was moved from original plan!)
// PA0-1,3-7: may get grounded by star rider board, so needs to be in input mode with no pull-ups
#define SETUP_PR8210_INPUT() DDRC &= ~((1 << PC0) | (1 << PC1) | (1 << PC6) | (1 << PC7)); \
	PORTC |= ((1 << PC0) | (1 << PC1) | (1 << PC6) | (1 << PC7)); \
	DDRD &= ~(1 << PD4); \
	PORTD |= (1 << PD4); \
	DDRA &= (1 << PA2); /* everything but PA2 in input mode, with pullups disabled */ \
	PORTA &= (1 << PA2)

// PC6 is PCINT22, which is part of PCIE2 (PCINT23..16); PC7 is PCINT23; PC0 is PCINT16,
//  we ignore PC1 for interrupt purposes because it is tied to PC0 by the star rider PIF board
#define SETUP_PR8210_INT() PCMSK2 = ((1 << PCINT16) | (1 << PCINT22) | (1 << PCINT23))

// PA2: video squelch
// PD5: stand by
#define SETUP_PR8210_OUTPUT()  DDRA |= (1 << PA2); \
	PORTA |= (1 << PA2); /* video squelch raised (inactive) by default */ \
	DDRD |= (1 << PD5); /* STAND-BY, low (inactive) by default */ \
	PORTD &= ~(1 << PD5)

// enable/disable PCINT2 interrupt
#define ENABLE_PR8210_INT() PCICR |= (1 << PCIE2)
#define DISABLE_PR8210_INT() PCICR &= ~(1 << PCIE2)

#define PR8210_JMPTRIG_INTEXT ((PINC >> 0) & 1)
#define PR8210_REMOTE_CTRL ((PINC >> 6) & 1)
#define PR8210_JMPTRIG ((PINC >> 7) & 1)
#define PR8210_SCANC ((PIND >> 4) & 1)

//////////////////////////////////

#define EEPROM_SIZE_BYTES 2048

#define FLASH_SIZE_BYTES 65536

// CRC-16 value stored as the last 2 bytes of application memory.  The bootloader takes up the last 2k, so subtract that first.
#define FLASH_CRC_OFFSET ((FLASH_SIZE_BYTES - 2048) - 2)
