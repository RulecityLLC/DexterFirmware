#ifndef STRINGS_H
#define STRINGS_H

typedef enum
{
	STRING_CRC_ERROR,
	STRING_UNKNOWN_PACKET,
	STRING_BAD_PACKET_XOR,
	STRING_PACKET_TOO_BIG,
	STRING_UNKNOWN_ERROR_ENUM,
	STRING_COUNT	/* this must come at the end */
} StringID;

void string_to_buf(char *s, StringID id);

#endif //  STRINGS_H
