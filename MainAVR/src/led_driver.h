#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <stdint.h>
#include "settings.h"
#include "rev.h"

// one time initialization
void leds_init();

void leds_clear_all();

void leds_change(uint8_t which, uint8_t bOn);

void leds_toggle(uint8_t which);

// call this during idle times
// TODO : make this an inline function
void leds_think();

uint8_t LDPTypeToLED(LDPType type);

// call when diagnose button has been pressed or when we power up for the first time
void update_diagnose_led();

#endif // LED_DRIVER_H
