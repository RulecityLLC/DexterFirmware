#ifndef VP931_MAIN
#define VP931_MAIN

#include "vldp-avr.h"
#include "vp931-common.h"

#define ENABLE_INT_WREN() EIMSK |= (1 << INT1)
#define DISABLE_INT_WREN() EIMSK &= ~(1 << INT1)

#define ENABLE_INT_RDEN() EIMSK |= (1 << INT0)
#define DISABLE_INT_RDEN() EIMSK &= ~(1 << INT0)

// handles both DAV' (PC1) and DAV (PC0)
#define LOWER_DAV_PRIME() PORTC &= ~(1 << PC1); PORTC |= (1 << PC0)
#define RAISE_DAV_PRIME() PORTC |= (1 << PC1); PORTC &= ~(1 << PC0)

// DAK is PD5
#define LOWER_DAK() PORTD &= ~(1 << PD5)
#define RAISE_DAK() PORTD |= (1 << PD5)

#define TIMER1_TIMED_OUT() (TIFR1 & (1 << OCF1A))
#define TIMER2_TIMED_OUT() (TIFR2 & (1 << OCF2A))

////////////////////////////////////////////

extern volatile uint8_t g_arrVP931StatusBytes[];	// the 6 status bytes that we send per field
extern volatile uint8_t g_u8VP931StatusBytesSent;
extern volatile uint8_t g_arrVP931CmdBytes[];	// the command bytes that we receive
extern volatile uint8_t g_u8VP931CmdBytesReceived;

void vp931_main_loop();
void vp931_rden_think();
void vp931_vsync_callback();
void vp931_timer1_callback();

#endif // VP931_MAIN
