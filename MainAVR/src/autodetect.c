#include <avr/io.h>
#include <string.h>	// strcat
#include "timer-global.h"
#include "idle.h"
#include "settings.h"
#include "protocol.h"
#include "dexter_strings.h"
#include "led_driver.h"
#include "ldv1000-main.h"
#include "pr7820-main.h"
#include "autodetect.h"

// how many times we read the same value before we decide that the value is stable
#define ITERATION_THRESHOLD 20

void ldv1000_or_pr7820_main_loop()
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
		bPin11Raised = IsPin11Raised();

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
		
			// stall? to give time for change to take effect
			//asm volatile("nop"::);

			// go back into input mode
			DDRC &= ~(1 << PC0);	// input mode for pin 11

			// now see if we stay low for a period
			// If we do, it means the line is probably in input mode on the other end and we are in LD-V1000 mode.
			bPin11Raised = IsPin11Raised();

			if (bPin11Raised)
			{
				bPr7820Mode = 1;
				break;
			}
		}
	} // end for loop

	if (bPr7820Mode)
	{
		probably_pr7820_mode();
	}
	else
	{
		probably_ldv1000_mode();
	}

	// when this method returns, we must have set the auto-detected type
}

void probably_ldv1000_mode()
{
	char s[30];	// "Auto-detected LD-V1000"
	char s1[10];
	string_to_buf(s, STRING_AUTODETECTED);
	string_to_buf(s1, STRING_LDV1000);
	strcat(s, s1);
	LOG(s);

	// TODO : we could get more fancy and send out some strobes as tests before changing ldp types...
	SetAutodetectedLDPType(LDP_LDV1000);
}

void probably_pr7820_mode()
{
	char s[30];
	char s1[10];
	string_to_buf(s, STRING_AUTODETECTED);
	string_to_buf(s1, STRING_PR7820);
	strcat(s, s1);
	LOG(s);

	// TODO : we could get more fancy and verify that PR-7820 is the actual type
	SetAutodetectedLDPType(LDP_PR7820);
}

uint8_t IsPin11Raised()
{
	uint8_t u8RaisedCounter, u8LoweredCounter;
	uint8_t bDone = 0;

	// go until we see the line consistently low or consistently high (no pull-up bounce)
	while (!bDone)
	{
		u8RaisedCounter = 0;
		u8LoweredCounter = 0;

		if ((PINC & (1 << PC0)) == 0)
		{
			u8RaisedCounter = 0;
			while ((PINC & (1 << PC0)) == 0)
			{
				u8LoweredCounter++;

				if (u8LoweredCounter == ITERATION_THRESHOLD)
				{
					bDone++;
					break;
				}
			}
		}
		else
		{
			u8LoweredCounter = 0;
			while ((PINC & (1 << PC0)) == 1)
			{
				u8RaisedCounter++;

				if (u8RaisedCounter == ITERATION_THRESHOLD)
				{
					bDone++;
					break;
				}
			}
		}
	}

	return u8RaisedCounter;
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
			
	// give time for the pull-ups to take effect
	idle_think();
				
	// if PC0 is connected to GND then it must be LD700 as that's the only player we currently support with that configuration
	if ((PINC & (1 << PC0)) == 0)
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
