// Definitions for Dexter rev1 board

// PORTA will be data lines (8 bits of data I/O)
#define DATA_CONTROL DDRA
#define DATA_PORT_WRITE PORTA
#define DATA_PORT_READ PINA

// vsync pin will be D2 (INT0)
// field pin will be D4
#define VSYNC_PIN ((PIND >> 2) & 1)
#define FIELD_PIN ((PIND >> 4) & 1)

// LED is plugged into this pin, call this macro to toggle it
#define LED_POWER_PIN_ACTIVE() (PORTD |= 0x40) /* PD6 */
#define LED_VSYNC_PIN_TOGGLE() (PORTD ^= 0x80)	/* PD7 */
#define LED_SETUP() (DDRD |= 0xC0)	/* put PD6 and PD7 in output mode */

#define ENABLE_VSYNC_INT() (EIMSK |= (1 << INT0))
#define DISABLE_VSYNC_INT() (EIMSK &= ~(1 << INT0))

// make interrupt occur on falling edge only
#define SETUP_VSYNC_INT() 	EICRA |= (1 << ISC01); EICRA &= ~(1 << ISC00)
