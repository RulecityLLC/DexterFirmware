#ifndef LDV1000SUPER_MAIN
#define LDV1000SUPER_MAIN

#include "common.h"

#include <avr/io.h> 
#include "settings.h"

// LD-V1000 'super' mode

// PC1 is pin7, PB6 is pin 17, put pin 11 (PC0) in input mode to be safe
#define LDV1000SUPER_SETUP_IO() DDRC |= (1 << PC1); PORTC |= (1 << PC1); \
	DDRC &= ~(1 << PC0); PORTC &= ~(1 << PC0); \
	DDRB &= ~(1 << PB6); PORTB &= ~(1 << PB6); DDRA = 0; PORTA = 0

#define LDV1000SUPER_IS_PIN17_RAISED() (PINB & (1 << PB6))
#define LDV1000SUPER_DISABLE_PIN7() PORTC |= (1 << PC1)
#define LDV1000SUPER_ENABLE_PIN7() PORTC &= ~(1 << PC1)

#define LDV1000SUPER_DATA_DRIVE_OUTPUT(u8) PORTA = u8; DDRA = 0xFF
#define LDV1000SUPER_DATA_GO_INPUT() PORTA = 0; DDRA = 0;
#define LDV1000SUPER_DATA_READ() PINA

#define MICROSECONDS_OF_SUPER_STATUS 25
#define CYCLES_OF_STATUS ((MY_F_CPU * MICROSECONDS_OF_SUPER_STATUS) / MILLION)
#define TICKS_FOR_STATUS (CYCLES_OF_STATUS/8)

////////////////////////////////////////////

void ldv1000super_main_loop();
void ldv1000super_process_cmd();

#endif // LDV1000SUPER_MAIN
