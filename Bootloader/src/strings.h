#ifndef STRINGS_H
#define STRINGS_H

typedef enum
{
	STRING_CRC_ERROR,
	STRING_BAD_PACKET_LENGTH,
	STRING_BAD_PAGE_IDX,
	STRING_HELLO,
	STRING_COUNT	/* this must come at the end */
} StringID;

void string_to_buf(char *s, StringID id);

#endif //  STRINGS_H
