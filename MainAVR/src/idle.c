#include "idle.h"
#include "led_driver.h"
#include "buttons.h"
#include "protocol.h"
#include "led_blinker.h"
#include "deferred_eeprom_write.h"
#include "timer-global.h"
#include "disc_switch.h"

void idle_think()
{
	io_think();
	leds_think();
	buttons_think();
	led_blinker_think();
	deferred_eeprom_write_think();
	timer_global_think();
	disc_switch_think();
}

void idle_think_critical_section()
{
	leds_think();
	buttons_think();
}
