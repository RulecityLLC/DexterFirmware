// ATMega644p stuff
#define EEPROM_SIZE_BYTES (2*1024)
#define PAGE_SIZE_BYTES 256
#define FLASH_SIZE_BYTES (64*1024)
#define BITS_PER_PAGE 8
#define TOTAL_PAGES 256-(2048/PAGE_SIZE_BYTES) /* 2048 is the size of this bootloader in bytes */
#define RX_INT_vectname USART0_RX_vect

// DIAGNOSE button on Dexter board
#define ALLOW_BUTTON_OVERRIDE

#ifdef REV3
// REV3 DIAGNOSE button is on PB3, external pull-up so disable internal pull-up
#define PREPARE_BUTTON() 	DDRB &= ~(1 << PB3); PORTB &= ~(1 << PB3)
#define IS_BUTTON_PRESSED()  ((PINB & (1 << PB3)) == 0)
#else
// REV2
// commented out so compiler warns us if we are accidentally building for REV2
//#define PREPARE_BUTTON() 	DDRB &= ~(1 << PB6); PORTB |= (1 << PB6)
//#define IS_BUTTON_PRESSED()  ((PINB & (1 << PB6)) == 0)
#endif

#define USART_BAUDRATE 115200
