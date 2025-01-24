#ifndef DISC_SWITCH_H
#define DISC_SWITCH_H

#include <stdint.h>

typedef enum
{
	DISC_SWITCH_IDLE,
	DISC_SWITCH_ACTIVE,
	DISC_SWITCH_SUCCESS,
	DISC_SWITCH_ERROR
} DiscSwitchStatus_t;

// processes stuff sent to us by mediaserver
void on_disc_switch_acknowledge(uint8_t u8DiscId, uint8_t bSuccess);

// initiates a new disc switch
void disc_switch_initiate(uint8_t idDisc);

// returns disc switch status
DiscSwitchStatus_t disc_switch_get_status();

// needs to be called regularly to move disc switching along.
void disc_switch_think();

// call this to acknowledge that you've seen a success/fail disc switch status.  This changes the status back to idle.
void disc_switch_end();

#endif // DISC_SWITCH_H