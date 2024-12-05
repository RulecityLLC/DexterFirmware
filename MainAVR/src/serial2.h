#ifndef SERIAL2_H
#define SERIAL2_H

#include <stdint.h>

void serial2_init(uint32_t u32BitsPerSecond);
void serial2_shutdown();

// Enables/disables TX; this is helpful if we know when the AUX AVR is in its 'critical' section and we don't want to send data during that time.
// If TX is disabled, data can still be buffered to be sent later.
void tx2_enable(uint8_t u8Enabled);

void tx2_to_buf(unsigned char ch);
unsigned char rx2_is_char_waiting();
unsigned char rx2_from_buf();

#endif // SERIAL2_H
