#include "led_driver.h"

// STP16CP05 LED driver

// rev2 wiring:
// SDI pin is connected to AVR MOSI pin
// CLK pin is connected to AVR SCK pin
// LE pin is connected to VCC
// SD0 pin is unconnected
// OE pin is connected to GND

// according to STP16CP05 timing diagram, SDI contains bit to be shifted in and CLK performs the actual shift.

#include <avr/io.h>
#include "rev.h"
#include "led_driver.h"

uint8_t g_bLedsNeedToUpdate = 0;

// 0 = not updating, 1 = updating first byte, 2 = updating second byte
uint8_t g_u8LedUpdateStage = 0;

// which leds are on, which are off (1 bit means on).
uint8_t g_u8LEDBits[2] = { 0, 0 };

void leds_init()
{
	// Set MOSI, SS and SCK output
	// IMPORTANT: according to datasheet, SS pin must be either set to output mode or must be held high in input mode.
	// Search for "SS pin functionality" section in datasheet for details.
	DDR_SPI |= (1<<DD_MOSI)|(1<<DD_SCK)|(1<<DD_SS);

	// Enable SPI, Master, set clock rate fck/4
	// (clock rate should run as fast as the LED driver chip can handle, which is 30 MHz)
	SPCR = (1<<SPE)|(1<<MSTR);

	// double clock rate to fck/2
	SPSR |= (1<<SPI2X);
}

void leds_clear_all()
{
	g_u8LEDBits[0] = 0;
	g_u8LEDBits[1] = 0;
	g_bLedsNeedToUpdate = 1;
}

void leds_change(uint8_t which, uint8_t bOn)
{
	uint8_t idx = 0;

	// if we are changing the second byte, modify index and 'which' value
	if (which > 7)
	{
		which &= 7;
		idx++;	// this (as I recall) is better for performance than "idx=1;"
	}

	if (bOn)
	{
		g_u8LEDBits[idx] |= (1 << which);
	}
	else
	{
		g_u8LEDBits[idx] &= ((1 << which) ^ 0xFF);
	}

	g_bLedsNeedToUpdate = 1;
}

void leds_toggle(uint8_t which)
{
	uint8_t idx = 0;

	// if we are changing the second byte, modify index and 'which' value
	if (which > 7)
	{
		which &= 7;
		idx++;	// this (as I recall) is better for performance than "idx=1;"
	}

	g_u8LEDBits[idx] ^= (1 << which);

	g_bLedsNeedToUpdate = 1;
}

// call this during idle times
// TODO : make this an inline function
void leds_think()
{
	// if we're in the middle of an led update
	if (g_u8LedUpdateStage != 0)
	{
		// if previous transmission has not completed yet
		// TODO : with some careful planning it should be possible to remove this check by ensuring that the previous transmission has always finished before this function gets called again
		if(!(SPSR & (1<<SPIF)))
		{
			return;
		}

		// if we need to send the second byte
		if (g_u8LedUpdateStage == 1)
		{
			// send the second byte
			SPDR = g_u8LEDBits[1];
			g_u8LedUpdateStage = 2;
		}
		// else if we just finished sending the second byte and are now done
		else
		{
			g_u8LedUpdateStage = 0;
		}
	}
	// else if we need to _do_ an update
	else if (g_bLedsNeedToUpdate)
	{
		// NOTE: do not need to check to see if previous transmission is still working because update stage can only be 0 if no work is being done

		// send the first byte
		SPDR = g_u8LEDBits[0];
		g_u8LedUpdateStage = 1;
		g_bLedsNeedToUpdate = 0;
	}
	// else don't need to do anything
}

uint8_t LDPTypeToLED(LDPType type)
{
	uint8_t res;

	switch(type)
	{
	default:	// ldv1000
		res = LED_LDV1000;
		break;
	case LDP_LDV1000_BL:
		res = LED_LDV1000_BL;
		break;
	case LDP_PR7820:
		res = LED_PR7820;
		break;
	case LDP_VP931:
		res = LED_VP931;
		break;
	case LDP_PR8210:
		res = LED_PR8210;
		break;
	case LDP_PR8210A:
		res = LED_PR8210A;
		break;
	case LDP_VP932:
		res = LED_VP932;
		break;
	case LDP_VP380:
		res = LED_VP380;
		break;
	case LDP_SIMUTREK:
		res = LED_SIMUTREK;
		break;
	case LDP_LDV8000:
		res = LED_LDV8000;
		break;
	case LDP_LDP1450:
		res = LED_LDP1450;
		break;
	case LDP_LDP1000A:	// before 'Other' had multiple modes, it was LDP1000A so we'll leave this here for safety reasons
	case LDP_OTHER:
		res = LED_OTHER;
		break;
	case LDP_VIP9500SG:
		res = LED_VIP9500SG;
		break;
	}

	return res;
}

void update_diagnose_led()
{
	// set DIAGNOSE LED if we're in diagnostics mode
	if (IsDiagnosticsEnabledEeprom())
	{
		leds_change(LED_DIAGNOSE, 1);
	}
	// else make sure the LED is cleared
	else
	{
		leds_change(LED_DIAGNOSE, 0);
	}
}
