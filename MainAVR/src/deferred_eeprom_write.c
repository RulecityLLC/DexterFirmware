#include "settings.h"
#include "common.h"
#include "timer-global.h"
#include "deferred_eeprom_write.h"

uint16_t g_u16DeferredEepromWriteSlowTimer = 0;

// whether we will write to the eeprom after the timer expires
uint8_t g_u8DeferredEepromWritePending = 0;

#define DEFERRED_EEPROM_WRITE_SECONDS 5

// slow timer is cpu frequency divided by 1024 (timer divider) and then 256 (8-bit range)
#define DEFERRED_EEPROM_WRITE_TICKS ((MY_F_CPU / 1024 / 256) * DEFERRED_EEPROM_WRITE_SECONDS)

void deferred_eeprom_write_restart()
{
	g_u8DeferredEepromWritePending = 1;
	g_u16DeferredEepromWriteSlowTimer = GET_GLOBAL_SLOW_TIMER_VALUE();
}

void deferred_eeprom_write_think()
{
	// if we are going to be writing to the eeprom in the future
	if (g_u8DeferredEepromWritePending)
	{
		uint16_t u16SlowTimerDiff = GLOBAL_SLOW_TIMER_DIFF_SINCE_START(g_u16DeferredEepromWriteSlowTimer);

		// is it time to write to the eeprom?
		if (u16SlowTimerDiff > DEFERRED_EEPROM_WRITE_TICKS)
		{
			save_current_settings_to_eeprom();
			g_u8DeferredEepromWritePending = 0;
		}
	}
}
	
