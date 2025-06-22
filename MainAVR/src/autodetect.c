#include <avr/io.h>
#include "dexter_strings.h"
#include "autodetect-deps.h"
#include "autodetect.h"

LDPType detect_ldv1000_or_pr7820()
{
	uint8_t bPin11Raised = 0;
	uint8_t bPr7820Mode = 0;
	uint8_t u8LoopCount = 0;

	// pin 17 = PB6	 (PR-7820 INT/EXT', LD-V1000 ENTER')
	// pin 11 = PC0	(PR-7820 ENTER,  LD-V1000 STATUS')
	// pin 7 = PC1	(PR-7820 READY, LD-V1000 CMD')

	DDRC |= (1 << PC1);	// output mode for pin 7
	PORTC |= (1 << PC1);	// make sure pin 7 is raised (disabled)

	DDRB &= ~(1 << PB6);	// input mode for pin 17
	PORTB &= ~(1 << PB6);	// make sure pull-up resistor is disabled

	for (u8LoopCount = 0; u8LoopCount < 2; u8LoopCount++)
	{
		DDRC &= ~(1 << PC0);	// input mode for pin 11
		PORTC |= (1 << PC0);	// set pull-up for pin 11

		// go until we see the line consistently low or consistently high (no pull-up bounce)
		bPin11Raised = IsPC0Raised();

		// if we saw the line consistently low for a period then we're in PR-7820 mode _for sure_ because we have a pull-up resistor enabled so we know the game PCB is in output mode
		if (!bPin11Raised)
		{
			bPr7820Mode = 1;
			break;
		}
		else
		{
			// go into output mode and force the line low, then examine how long it takes for it to go high again
			// (yes, this is rough on the hardware, which is why we should only do this auto detection sparingly)

			PORTC &= ~(1 << PC0);	// prepare to force low
			DDRC |= (1 << PC0);	// output mode for pin 11
			DDRC &= ~(1 << PC0);	// go immediately back into input mode for pin 11

			// now see if we stay low for a period
			// If we do, it means the line is probably in input mode on the other end and we are in LD-V1000 mode.
			bPin11Raised = IsPC0Raised();

			if (bPin11Raised)
			{
				bPr7820Mode = 1;
				break;
			}
		}
	} // end for loop

	return (bPr7820Mode != 0) ? LDP_PR7820 : LDP_LDV1000;
}

LDPType detect_other_mode()
{
	// 'Other' mode examines some pins to determine which adapter is plugged into the DB25 port
	// PC1 (DB12) and PC0 (DB11) values when read with pull-ups enabled
	// 11: LDP-1000A
	// 10: LD700 (Halcyon)
	LDPType detectedType;
	uint8_t u8DDRCSaved = DDRC;
	uint8_t u8PORTCSaved = PORTC;
	DDRC &= ~((1 << PC0) | (1 << PC1));	// go into input mode for these pins
	PORTC |= ((1 << PC0) | (1 << PC1));	// enable pull-ups
							
	// if PC0 is connected to GND then it must be LD700 as that's the only player we currently support with that configuration
	if (IsPC0Raised() == 0)
	{
		detectedType = LDP_LD700;
	}
	// else the pin is raised, so the only player we currently support is LDP-1000A
	else
	{
		detectedType = LDP_LDP1000A;
	}
				
	// clean-up after ourselves
	PORTC = u8PORTCSaved;
	DDRC = u8DDRCSaved;
				
	return detectedType;
}
