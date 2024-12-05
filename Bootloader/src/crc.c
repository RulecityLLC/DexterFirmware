
#include "crc.h"

uint16_t g_crc;

///////

void
crc_ccitt_init(void)
{
	g_crc = 0xffff;	// 0xFFFF initialization standard CCITT
}

// see http://www.lammertbies.nl/comm/info/crc-calculation.html
//  and http://zorc.breitbandkatze.de/crc.html
//  to verify the accuracy of this algorithm.
void
crc_ccitt_update(unsigned char x)
{
	uint16_t crc_new = (unsigned char)(g_crc >> 8) | (g_crc << 8);
	crc_new ^= x;
	crc_new ^= (unsigned char)(crc_new & 0xff) >> 4;
	crc_new ^= crc_new << 12;
	crc_new ^= (crc_new & 0xff) << 5;
	g_crc = crc_new;
}

uint16_t
crc_ccitt_crc(void)
{
	return g_crc;
}
