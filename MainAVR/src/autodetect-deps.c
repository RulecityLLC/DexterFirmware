#include <avr/io.h>
#include "idle.h"
#include "autodetect-deps.h"

// how many times we read the same value before we decide that the value is stable
#define ITERATION_THRESHOLD 20

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
