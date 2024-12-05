#include "protocol.h"
#include "strings.h"
#include <avr/pgmspace.h>

// IMPORTANT: keep these strings shorter than the buffer in log_string!!
const char strCRCError[] PROGMEM = "CRC err";
const char strBadPacketLength[] PROGMEM = "Bad len";
const char strBadPageIdx[] PROGMEM = "Bad idx";
const char strHello[] PROGMEM = "Bootloader v00";

PGM_P const string_table[] PROGMEM = 
{
	strCRCError,
	strBadPacketLength,
	strBadPageIdx,
	strHello
};

void string_to_buf(char *s, StringID id)
{
	strcpy_P(s, (PGM_P)pgm_read_word(&(string_table[id])));
}
