#ifndef LDV1000_MAIN
#define LDV1000_MAIN

#include "vldp-avr.h"
#include "settings.h"
#include "ldv1000-common.h"

// LD-V1000

// AVR pinouts (this is the same for REV1, REV2, and REV3):
// PORTC0 will be strobe
// PORTC1 will be command

#define STROBE_PORT_WRITE PORTC
#define STROBE_PORT_READ PINC
#define STATUS_BIT PC0
#define CMD_BIT PC1

// put cmd and status strobe in output mode
#define SETUP_STROBES() (DDRC |= ((1 << STATUS_BIT) | (1 << CMD_BIT)))

// both strobes are ACTIVE LOW
#define DISABLE_STATUS_STROBE() STROBE_PORT_WRITE |= (1 << STATUS_BIT)
#define ENABLE_STATUS_STROBE() STROBE_PORT_WRITE &= ~(1 << STATUS_BIT)
#define DISABLE_CMD_STROBE() STROBE_PORT_WRITE |= (1 << CMD_BIT)
#define ENABLE_CMD_STROBE() STROBE_PORT_WRITE &= ~(1 << CMD_BIT)

////////////////////////////////////////////

void ldv1000_main_loop(LDPType type);	// ldptype passed in to avoid knowledge of whether we auto-detected the player or not
void ldv1000_vsync_callback();
void ldv1000_timer1_callback();

#endif // LDV1000_MAIN
