#include "serial_packet.h"
#include <string.h>	// for memset

SerialPacketContext_t g_ctx;

// CALLER MUST DEFINE THESE
void (*g_serpck_on_good_packet)() = 0;
void (*g_serpck_error)(SerialPacketErrCode_t code, uint16_t u16Val1, uint16_t u16Val2) = 0;

void serpck_crc_ccitt_init(void)
{
	g_ctx.crc_calculated = 0xffff;	// 0xFFFF initialization standard CCITT
}

// see http://www.lammertbies.nl/comm/info/crc-calculation.html
//  and http://zorc.breitbandkatze.de/crc.html
//  to verify the accuracy of this algorithm.
void serpck_crc_ccitt_update(unsigned char x)
{
	uint16_t crc_new = (unsigned char)(g_ctx.crc_calculated >> 8) | (g_ctx.crc_calculated << 8);
	crc_new ^= x;
	crc_new ^= (unsigned char)(crc_new & 0xff) >> 4;
	crc_new ^= crc_new << 12;
	crc_new ^= (crc_new & 0xff) << 5;
	g_ctx.crc_calculated = crc_new;
}

SerialPacketContext_t *serpck_get_context()
{
	return &g_ctx;
}

void serpck_reset_context()
{
	memset(&g_ctx, 0, sizeof(g_ctx));
	serpck_crc_ccitt_init();
}

// call this regularly
void serpck_process_byte(uint8_t ch)
{
	// if we are waiting for the sync byte
	if (!g_ctx.bGotSyncByte)
	{
		// If we get a 0, it means we've found the beginning of a new packet
		//  so we are _probably_ in sync (CRC could contain a 0 for example, so this is not guaranteed)
		// If we aren't in sync, this will definitely help us get in sync until we eventually are in sync again.
		if (ch != 0)
		{
			g_ctx.u16RawIdx = 0;
			return;	// don't increment RawIdx
		}

		// this is good place to reset the context, because it gives the caller time to retrieve the previous packet
		serpck_reset_context();

		g_ctx.bGotSyncByte = 1;
	}
	// grab first 4 bytes to check to see if we've really found beginning of a legit packet
	else if (g_ctx.u16RawIdx < 4)
	{
		g_ctx.prefixBuf[g_ctx.u16RawIdx] = ch;
	}
	// else if we're waiting for the second XOR byte of the packet length
	else if (g_ctx.u16RawIdx == 4)
	{
		uint16_t u16PacketLengthXor = (g_ctx.prefixBuf[3] | (ch << 8));
		g_ctx.u16PacketLength = (g_ctx.prefixBuf[1] | (g_ctx.prefixBuf[2] << 8));

		// the second word MUST be the XOR of the first word (so we know the packet length is correct)
		if (g_ctx.u16PacketLength == (u16PacketLengthXor ^ 0xFFFF))
		{
			// check for packet size being too big
			if (g_ctx.u16PacketLength > MAX_PACKET_SIZE)
			{
				g_serpck_error(SERPCK_ERR_PACKET_TOO_BIG, g_ctx.u16PacketLength, MAX_PACKET_SIZE);
				g_ctx.bGotSyncByte = 0;	// prepare to reset context
				return;
			}
			// else
			// everything is ok
		}
		// else XOR does not match, so check the rest of the packet
		else
		{
				uint8_t i = 1;	// skip over first byte which may be a false sync byte

				// look within prefix buf to see if any of the other bytes could be a sync byte
				while (i < sizeof(g_ctx.prefixBuf))
				{
					if (g_ctx.prefixBuf[i] == 0)
					{
						break;
					}
					i++;
				}

				// if we didn't find a sync byte, we're done..
				if (i == sizeof(g_ctx.prefixBuf))
				{
					// if the last byte received is a sync byte
					if (ch == 0)
					{
						g_ctx.prefixBuf[0] = ch;
						g_ctx.u16RawIdx = 1;
					}
					// else we can't make use of anything we've received so far.  Throw error to make troubleshooting easier.
					else
					{
						g_serpck_error(SERPCK_ERR_BAD_XOR, g_ctx.u16PacketLength, u16PacketLengthXor);
						g_ctx.bGotSyncByte = 0;	// prepare to reset context
					}
					return;
				}
				// else we found another possible candidate, consider it the next time we receive a byte
				else
				{
					uint8_t j = 0;

					while (i < sizeof(g_ctx.prefixBuf))
					{
						g_ctx.prefixBuf[j++] = g_ctx.prefixBuf[i++];
					}
					g_ctx.prefixBuf[j++] = ch;
					g_ctx.u16RawIdx = j;
					return;
				}
		}
	}
	// else if we are in the middle of receiving the packet
	else if (g_ctx.u16PacketIdx < g_ctx.u16PacketLength)
	{
		g_ctx.pPacketBuf[g_ctx.u16PacketIdx++] = ch;
		serpck_crc_ccitt_update(ch);
	}
	// else if we are receiving the CRC at the end of the packet
	else if (g_ctx.u8CRCIdx < 2)
	{
		g_ctx.crc_received[g_ctx.u8CRCIdx++] = ch;

		// if we've received the full packet, then process it
		if (g_ctx.u8CRCIdx == 2)
		{
			// lil endian
			uint16_t u16CrcReceived = (g_ctx.crc_received[1] << 8) | g_ctx.crc_received[0];

			// crc checking belongs here since other parts of the code don't necessarily need to know what the crc algorithm is; they only care about the packet contents
			if (g_ctx.crc_calculated != u16CrcReceived)
			{
				// NOTE : if we get a CRC error, we won't try to find a new sync byte because we have already passed the XOR check, which suggests that we indeed found the correct sync byte and that the error is due to corruption on the serial lines.
				g_serpck_error(SERPCK_ERR_BAD_CRC, u16CrcReceived, g_ctx.crc_calculated);
			}
			else
			{
				g_serpck_on_good_packet();
			}
			
			g_ctx.bGotSyncByte = 0;	// give caller time to grab the packet contents before we clobber stuff
			return; 
		}
	}

	// this makes it convenient to figure out where we are
	g_ctx.u16RawIdx++;
}
