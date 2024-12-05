// ATMega328p stuff
#define EEPROM_SIZE_BYTES (1*1024)
#define PAGE_SIZE_BYTES 128
#define FLASH_SIZE_BYTES (32*1024)
#define BITS_PER_PAGE 7
#define TOTAL_PAGES 256-(2048/PAGE_SIZE_BYTES) /* 2048 is the size of this bootloader in bytes */
#define RX_INT_vectname USART_RX_vect

#ifdef REV3

// DIAGNOSE button on Dexter board is tied to both main and aux AVRs with external pull-up
#define ALLOW_BUTTON_OVERRIDE

// REV3 DIAGNOSE line is on PB1 for AUX AVR, external pull-up so disable internal pull-up.
// Setup the MUX IC while we are here to make sure it is in a good state.
#define PREPARE_BUTTON() 	DDRB &= ~(1 << PB1); PORTB &= ~(1 << PB1); DDRB |= (1 << PB3) | (1 << PB4); PORTB |= ((1 << PB3) | (1 << PB4))
#define IS_BUTTON_PRESSED()  ((PINB & (1 << PB1)) == 0)
#else
// REV2
// not supported on VBI injector board
#endif

// works better at 16 mhz
#define USART_BAUDRATE 76800

// for rev3a boards
//#define VBI_INJECT_DISABLE
