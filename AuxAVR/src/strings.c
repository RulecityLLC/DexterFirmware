#include "strings.h"
#include <avr/pgmspace.h>

// NOTE : these strings are terse due to memory limitations of the main AVR (not the aux AVR)
const char strCRCError[] PROGMEM = "CRC %04x %04x";
const char strUnknownPacket[] PROGMEM = "Unknwn pckt %02x";
const char strBadPacketLength[] PROGMEM = "XOR %04x %04x";
const char strPacketTooBig[] PROGMEM = "Too big %u";
const char strUnknownErrEnum[] PROGMEM = "Enum %02x";

PGM_P const string_table[] PROGMEM = 
{
	strCRCError,
	strUnknownPacket,
	strBadPacketLength,
	strPacketTooBig,
	strUnknownErrEnum
};

void string_to_buf(char *s, StringID id)
{
	strcpy_P(s, (PGM_P)pgm_read_word(&(string_table[id])));
}
