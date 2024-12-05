#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

// hand-optimized version of itoa that converts an unsigned byte to a hex string
// does NOT null-terminate!
void ByteToHexString(char *pDst, uint8_t u8Src);

// hand-optimized version of itoa that converts an uint16_t to a hex string
// does NOT null-terminate!
void Uint16ToHexString(char *pDst, uint16_t u16Src);

#endif // UTIL_H
