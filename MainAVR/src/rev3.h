// Definitions for dexter rev2 board

// PORTA will be data lines (8 bits of data I/O)
#define DATA_CONTROL DDRA
#define DATA_PORT_WRITE PORTA
#define DATA_PORT_READ PINA

// field pin will be B2 (INT2)
// there is no longer a dedicated vsync pin (field pin change is used to detect vsync)
// The LM1881 uses 0 for even/bottom and 1 for top/odd (backward from our convention)
#define FIELD_PIN ((PINB >> 2) & 1)

#define VSYNC_EXT_INT INT2

// make interrupt occur on falling AND rising edge of FIELD pin
#define SETUP_VSYNC_INT() EICRA &= ~(1 << ISC21); EICRA |= (1 << ISC20)

// if we aren't using ISR's for vsync, these macros will work for C code
#define GOT_VSYNC() (EIFR & (1 << INTF2))

// clear vsync flag (AVR requires writing 1)
#define CLEAR_VSYNC_FLAG() EIFR |= (1 << INTF2)

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
	DDRD &= ~(1 << PD7);	/* set MODE line to input mode */ \
	PORTD |= (1 << PD7);	/* pull-up because there is NOT one on the board */ \
	DDRB &= ~(1 << PB3);	/* set DIAGNOSE line to input mode */ \
	PORTB &= ~(1 << PB3);	/* no pull-up because there is one on the board */

#define BUTTON_DIAGNOSE (PINB & (1 << PB3))
#define BUTTON_MODE (PIND & (1 << PD7))

///////////////////////////////////////////////////////////////

// rev3 has no s-video port so this does nothing
#define SETUP_SVIDEO_MUTE()

///////////////////////////////////////////////////////////////

// PB6 (ENTER) needs to be in input mode with pull-up enable to match LD-V1000 hardware
#define SETUP_LDV1000_ENTER() DDRB &= ~(1 << PB6); PORTB |= (1 << PB6)
#define IS_LDV1000_ENTER_RAISED() (PINB & (1 << PB6))

// pin 12 (PD5) is supposed to be grounded; we set it to input with no pull-up so that it doesn't conflict with other hardware such as PR-8210A
#define SETUP_LDV1000_GND() DDRD &= ~(1 << PD5); PORTD &= ~(1 << PD5)

///////////////////////////////////////////////////////////////

// pin7 (ready') is PC1 [output]
// pin11 (enter') is PC0/PCINT16 [input]
// pin17 (INT/EXT') is PB6/PCINT14 [input]
// For now, we only care about the enter line and ignore the int/ext' line
#define SETUP_PR7820() DDRC |= (1 << PC1); DDRC &= ~(1 << PC0); \
	PORTC &= ~(1 << PC0); DDRB &= ~(1 << PB6); \
	PORTB &= ~(1 << PB6); \
	PCMSK2 = (1 << PCINT16)

#define PR7820_RAISE_READY_PRIME() PORTC |= (1 << PC1)
#define PR7820_LOWER_READY_PRIME() PORTC &= ~(1 << PC1)
#define PR7820_ENTER_PRIME() (PINC & (1 << PC0))
#define PR7820_INT_EXT_PRIME() (PINB & (1 << PB6))

///////////////////////////////////////////////////////////////

// LED mappings

#define LED_LDP1450 2
#define	LED_VIP9500SG 3
#define LED_OTHER 4
#define	LED_PR8210 6
#define	LED_DIAGNOSE 7
#define	LED_VIDEO_OK 8
#define	LED_LDV1000 9
#define	LED_LDV1000_BL 10
#define	LED_PR7820 11
#define	LED_PR8210A 12
#define	LED_VP932 14
#define	LED_VP931 15
#define LED_SIMUTREK 13

// not indicated on the PCB for this revision
#define LED_VP380 4
#define LED_LDV8000 4
#define LED_LDP1000A 4

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

// PR-8210/A stuff

// Set (PR8210 int/ext', remote control, scan c, and jmptrig') to input mode and raise pull-up resistors for them all
// PC0: Jmp Trigger INT/EXT' (tied to Scan C INT/EXT')
// PC1: Scan C INT/EXT'
// PC6: remote control
// PD5: Jmp trigger
// PD4: Scan C (this was moved from original plan!)
// PA0: vsync, unconnected (generated externally)
// PA1: vbi data, unconnected by PIF board but we can use it to troubleshoot with a logic analyzer
// PA2: video squelch'
// PA3: csync, unconnected (generated externally)
// PA4-PA7: GND by star rider PIF board, needs to be in input mode without pull-ups
// PA0-1,3-7: may get grounded by star rider board, so needs to be in input mode with no pull-ups

#define SETUP_PR8210_INPUT() DDRC &= ~((1 << PC0) | (1 << PC1) | (1 << PC6)); \
	PORTC |= ((1 << PC0) | (1 << PC1) | (1 << PC6)); \
	DDRD &= ~((1 << PD4) | (1 << PD5)); \
	PORTD |= (1 << PD4) | (1 << PD5); /* enable internal pull-ups just so we don't have floating inputs */ \
	DDRA = ((1 << PA1)|(1 << PA2)); /* everything but PA1/PA2 in input mode, with pullups disabled */ \
	PORTA = 0

// PC6 is PCINT22, which is part of PCIE2 (PCINT23..16); PD5 is PCINT29; PC0 is PCINT16,
//  we ignore PC1 for interrupt purposes because it is tied to PC0 by the star rider PIF board
#define SETUP_PR8210_INT() PCMSK2 = ((1 << PCINT16) | (1 << PCINT22)); \
	PCMSK3 = (1 << PCINT29)

// PA2: video squelch
// PC7: stand by
#define SETUP_PR8210_OUTPUT()  DDRA |= (1 << PA2); \
	PORTA |= (1 << PA2); /* video squelch raised (inactive) by default */ \
	DDRC |= (1 << PC7); /* STAND-BY, low (inactive) by default */ \
	PORTC &= ~(1 << PC7)

// enable/disable PCINT2 interrupt
#define ENABLE_PR8210_INT() PCICR |= (1 << PCIE2)|(1 << PCIE3)
#define DISABLE_PR8210_INT() PCICR &= ~((1 << PCIE2)|(1 << PCIE3))

// PR-8210A line (PD6) is active low
// (setting the line to output as part of this macro is intentional because we want to be able to disable this mode without enabling other PR-8210 lines)
#define ENABLE_PR8210A_MODE() DDRD |= (1 << PD6); PORTD &= ~(1 << PD6)
#define DISABLE_PR8210A_MODE() DDRD |= (1 << PD6); PORTD |= (1 << PD6)

#define LOWER_PR8210A_VBI_DATA() PORTA &= ~(1 << PA1)
#define RAISE_PR8210A_VBI_DATA() PORTA |= (1 << PA1)

#define ENABLE_PR8210A_SQUELCH() PORTA &= ~(1 << PA2)
#define DISABLE_PR8210A_SQUELCH() PORTA |= (1 << PA2)

// PR-8210A stand by moved to AUX AVR for rev3
#define ENABLE_PR8210A_STANDBY() g_bPR8210AStandByRaised = 1
#define DISABLE_PR8210A_STANDBY() g_bPR8210AStandByRaised = 0

#define PR8210_JMPTRIG_INTEXT ((PINC >> 0) & 1)
#define PR8210_REMOTE_CTRL ((PINC >> 6) & 1)
#define PR8210_JMPTRIG ((PIND >> 5) & 1)
#define PR8210_SCANC ((PIND >> 4) & 1)

//////////////////////////////////

// PC0: RTS
// PC1: DSR

#define SETUP_VP932()	DDRC &= ~(1 << PC1); DDRC |= (1 << PC0)

#define VP932_DSR() (PINC & (1 << PC1))

#define VP932_RAISE_RTS() (PORTC |= (1 << PC0))
#define VP932_LOWER_RTS() (PORTC &= ~(1 << PC0))

//////////////////////////////////

#define EEPROM_SIZE_BYTES 2048

#define FLASH_SIZE_BYTES 65536

// CRC-16 value stored as the last 2 bytes of application memory.  The bootloader takes up the last 2k, so subtract that first.
#define FLASH_CRC_OFFSET ((FLASH_SIZE_BYTES - 2048) - 2)
