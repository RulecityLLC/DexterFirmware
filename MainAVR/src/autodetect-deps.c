#include <avr/io.h>
#include "idle.h"
#include "autodetect-deps.h"

// how many times we read the same value before we decide that the value is stable
#define ITERATION_THRESHOLD 20

uint8_t IsPC0Raised()
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
