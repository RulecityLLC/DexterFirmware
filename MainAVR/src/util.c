#include "util.h"

// hand-optimized version of itoa that converts an unsigned byte to a hex string
// does NOT null-terminate!
void ByteToHexString(char *pDst, uint8_t u8Src)
{
	const char *DIGITS = "0123456789ABCDEF";

	pDst[1] = DIGITS[(u8Src & 0xF)];
	u8Src >>= 4;
	pDst[0] = DIGITS[(u8Src)];
}

// hand-optimized version of itoa that converts an uint16_t to a hex string
// does NOT null-terminate!
void Uint16ToHexString(char *pDst, uint16_t u16Src)
{
	const char *DIGITS = "0123456789ABCDEF";

	pDst[3] = DIGITS[(u16Src & 0xF)];
	u16Src >>= 4;
	pDst[2] = DIGITS[(u16Src & 0xF)];
	u16Src >>= 4;
	pDst[1] = DIGITS[(u16Src & 0xF)];
	u16Src >>= 4;
	pDst[0] = DIGITS[(u16Src)];
}
