#include "led_driver.h"
#include "settings.h"
#include "common.h"
#include "timer-global.h"
#include "led_blinker.h"

uint16_t g_u16LedBlinkerSlowTimer = 0;
uint8_t g_bLedBlinkerLedOn = 0;	// 1 = the blinking led is on, 0 = the blinking led is off

// Coupling warning: Will this cause some LEDs to be left on when the mode is changed?  I believe it will not since when the mode is changed, all leds are disabled and the active LDP type is set.

// slow timer is cpu frequency divided by 1024 (timer divider) and then 256 (8-bit range)
#define LED_BLINKER_ON_TICKS ((MY_F_CPU / 1024 / 256) * 0.5)
#define LED_BLINKER_OFF_TICKS ((MY_F_CPU / 1024 / 256) * 0.0625)

void led_blinker_think()
{
	// if auto-detection is disabled, we blink the active player type to show this
	if (!IsAutoDetectionEnabledEeprom())
	{
		uint16_t u16SlowTimerDiff = GLOBAL_SLOW_TIMER_DIFF_SINCE_START(g_u16LedBlinkerSlowTimer);
		LDPType ldpType = GetActiveLDPType();

		// if the led is currently on, is it time for us to blink it off?
		if (g_bLedBlinkerLedOn)
		{
			// yes, blink it off
			if (u16SlowTimerDiff > LED_BLINKER_ON_TICKS)
			{
				leds_change(LDPTypeToLED(ldpType), 0);
				g_bLedBlinkerLedOn = 0;
				g_u16LedBlinkerSlowTimer = GET_GLOBAL_SLOW_TIMER_VALUE();
			}
		}
		// else if it's off, is it time to turn it back on?
		else
		{
			// yes, blink it on
			if (u16SlowTimerDiff > LED_BLINKER_OFF_TICKS)
			{
				leds_change(LDPTypeToLED(ldpType), 1);
				g_bLedBlinkerLedOn = 1;
				g_u16LedBlinkerSlowTimer = GET_GLOBAL_SLOW_TIMER_VALUE();
			}
		}
	}
}
	
