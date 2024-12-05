#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "bootloader.h"
#include "platform.h"
#include "protocol.h"
#include "program.h"
#include "serial.h"
#include <avr/sleep.h>

int main(void)
{
	// if we are completely disabling the injector
#ifdef VBI_INJECT_DISABLE
	DDRB |= (1 << PB3) | (1 << PB4);

	// disable white
	PORTB |= (1 << PB4);

	// disable video mute
	PORTB |= (1 << PB3);

	// halt the cpu
	for (;;)
	{
		cli();
		sleep_enable();
		sleep_cpu();
		// endless loop
	}
#endif

#ifdef ALLOW_BUTTON_OVERRIDE
	// get ready to read button
	PREPARE_BUTTON();
#endif

	// setup 16-bit timer to divide by 1024 (we will want to be delaying a second or two if possible)
	// Setup CTC to trigger when timer hits its boundary.
	TCCR1B = (1 << CS12) | (1 << CS10) | (1 << WGM12);
	OCR1A = 46875;	// about 3 seconds at 16 mhz (16,000,000 / 1024) * 3

	// read last 2 bytes in the EEPROM to determine if we need to program or not
	uint16_t u16Token = eeprom_read_word (( uint16_t *) EEPROM_SIZE_BYTES - 2);

	// if we find the token we are expecting,
	if (u16Token == 0xBEEF)
	{
// can a button being depressed during reset force the bootloader to reprogram?
#ifdef ALLOW_BUTTON_OVERRIDE
		TCNT1 = 0;	// start watching the clock
		TIFR1 |= ((1 << OCF1A) | (1 << TOV1));	// reset CTC

		// wait for a long time before we initiate reprogram.  If button is released at any time, do not reprogram. (error on side of caution)
		while (IS_BUTTON_PRESSED())
		{
			// if it's been long enough, initiate reprogramming
			if (TIFR1 & (1 << OCF1A))
			{
				goto reprogram;
			}
		}

		// if we get this far, it means that button was released so do not reprogram
#endif

		goto done;
	}

reprogram:
	serial_init();
	PROGRAM_INIT();

	// tell AVR to use interrupt table of bootloader instead of the one at $0
	MCUCR = (1<<IVCE);
	MCUCR = (1<<IVSEL);

	// enable interrupts so we can start receiving RX data
	sei();

	// give a hello message for visibility
	LogString(STRING_HELLO);

	// bootstrap the page request loop
	RequestNextPage();

	while (IS_PROGRAMMING_ACTIVE())
	{
		io_think();
	}

	// write to eeprom to indicate programming has finished
	eeprom_update_word((uint16_t *) EEPROM_SIZE_BYTES - 2, 0xBEEF);

	// get back into a somewhat default state
	cli();
	SERIAL_SHUTDOWN();

	// set interrupt table back to $0
	MCUCR = (1<<IVCE);
	MCUCR = 0;

done:

	// revert timer registers to reset defaults
	TCCR1B = 0;
	OCR1A = 0;
	TIFR1 |= ((1 << OCF1A) | (1 << TOV1));	// just in case

	// boot program
	asm volatile("jmp 0x0000"::);
}
