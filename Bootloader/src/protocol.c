#include <avr/io.h>
#include "serial.h"
#include <string.h>
#include "crc.h"
#include "protocol.h"
#include "platform.h"
#include "program.h"

// max packet size at the moment (where 644p is the largest AVR we will support)
#define MAX_PACKET_SIZE 256+3

uint8_t g_bGotSyncByte = 0;
uint16_t g_u16RawIdx = 0;
uint16_t g_u16PacketLength = 0;
uint8_t g_u8PacketXorTmp = 0;
uint16_t g_u16PacketIdx = 0;
uint8_t g_pPacketBuf[MAX_PACKET_SIZE];
uint8_t g_u8CRCIdx = 0;
uint8_t g_crc[2];
uint8_t g_u8NextPageToRequest = 0;
uint8_t g_u8LastPageRequested = 0;
uint8_t g_u8PageReRequestCount = 0;	// how many times page has been re-requested

////////////////////////////////

// call this regularly
void io_think()
{
	// if we've timed out and need to send another request
	if (TIFR1 & (1 << OCF1A))
	{
		g_u8PageReRequestCount++;

		// if the peer has gone silent, start over from the beginning.
		// This is to minimize risk of getting two or more different versions of the firmware update.
		if (g_u8PageReRequestCount & 32)
		{
			g_u8LastPageRequested = 0;
			g_u8NextPageToRequest = 1;
		}
		SendPageReRequest();
	}

	// if no character is waiting in the receive buffer, we're done
	if (!rx_is_char_waiting())
	{
		// empty output buffer
		tx_think();
		return;
	}

	// handle 1 byte from receive buffer
	uint8_t ch = rx_from_buf();

	// if we are waiting for the sync byte
	if (!g_bGotSyncByte)
	{
		// If we get a 0, it means we've found the beginning of a new packet
		//  so we are _probably_ in sync (CRC could contain a 0 for example, so this is not guaranteed)
		// If we aren't in sync, this will definitely help us get in sync until we eventually are in sync again.
		if (ch != 0)
		{
			g_u16RawIdx = 0;
			return;	// don't increment RawIdx
		}

		g_bGotSyncByte = 1;
	}
	// else if we're waiting for the first byte of the packet length
	else if (g_u16RawIdx == 1)
	{
		g_u16PacketLength = ch;
	}
	// else if we're waiting for the second byte of the packet length
	else if (g_u16RawIdx == 2)
	{
		g_u16PacketLength = (ch << 8) | g_u16PacketLength;
	}
	// else if we're waiting for the first XOR byte of the packet length
	else if (g_u16RawIdx == 3)
	{
		g_u8PacketXorTmp = ch;
	}
	// else if we're waiting for the second XOR byte of the packet length
	else if (g_u16RawIdx == 4)
	{
		// the second byte MUST be the XOR of the first byte (so we know the packet length is correct)
		if (g_u16PacketLength == ((g_u8PacketXorTmp | (ch << 8)) ^ 0xFFFF))
		{
			// everything is ok
		}
		else
		{
			LogString(STRING_BAD_PACKET_LENGTH);
			goto reset;
		}
	}
	// else if we are in the middle of receiving the packet
	else if (g_u16PacketIdx < g_u16PacketLength)
	{
		g_pPacketBuf[g_u16PacketIdx++] = ch;
	}
	// else if we are receiving the CRC at the end of the packet
	else if (g_u8CRCIdx < 2)
	{
		g_crc[g_u8CRCIdx++] = ch;

		// if we've received the full packet, then process it
		if (g_u8CRCIdx == 2)
		{
			ProcessPacket();

reset:
			g_bGotSyncByte = 0;
			g_u16PacketLength = 0;
			g_u16PacketIdx = 0;
			g_u8CRCIdx = 0;
			g_u16RawIdx = 0;
			g_u8PacketXorTmp = 0;
			return;	// don't increment rawIdx
		}
	}

	// this makes it convenient to figure out where we are
	g_u16RawIdx++;

}

void ProcessPacket()
{
	// check CRC first
	crc_ccitt_init();
	for (uint16_t i = 0; i < g_u16PacketLength; i++)
	{
		crc_ccitt_update(g_pPacketBuf[i]);
	}

	uint16_t u16ActualCRC = crc_ccitt_crc();
	uint16_t u16ReceivedCRC = g_crc[0] | (g_crc[1] << 8);	// little endian

	// if CRC does not match
	if (u16ReceivedCRC != u16ActualCRC)
	{
		LogString(STRING_CRC_ERROR);
		// let timer trigger page re-request so that we don't flood peer
		return;
	}

	// get command code
	unsigned char chCmd = g_pPacketBuf[0];

	switch (chCmd)
	{
	default:
		// It's important to ignore unknown packets rather than spam "unknown packet" errors.
		// If we spam unknown packet errors, we may flood I/O such that we never get known packets.
		break;
	case 'P':	// diagnostics change acknowledge

		// make sure packet length is what we expect it to be
		if (g_u16PacketLength == (PAGE_SIZE_BYTES + 3))
		{
			uint8_t u8Page = g_pPacketBuf[1];	// for now we only support up to 256 pages, so we can ignore second byte

			// if we have received the proper page
			if (u8Page == g_u8LastPageRequested)
			{
				program_page(u8Page << BITS_PER_PAGE, &g_pPacketBuf[3]);

				// if we still have more pages to request
				if (g_u8NextPageToRequest < TOTAL_PAGES)
				{
					RequestNextPage();
				}
				// else we're done!
				else
				{
					PROGRAM_END();	// stop programming
				}
			}
			else
			{
				LogString(STRING_BAD_PAGE_IDX);
				// let timer trigger page re-request so that we don't flood peer
			}
		}
		else
		{
			LogString(STRING_BAD_PACKET_LENGTH);
			// let timer trigger page re-request so that we don't flood peer
		}
		break;
	}
}

void LogString(StringID id)
{
	char s[30];
	string_to_buf(s, id);
	SendLog(s);
}

///////////////////////////////////////////

void SendBuf(unsigned char *pBuf, uint8_t u8Length)
{
	unsigned int uCRC = 0;

	crc_ccitt_init();

	tx_to_buf(0);	// sync byte
	tx_to_buf(u8Length);	// length LSB
	tx_to_buf(u8Length ^ 0xFF);	// xor of length LSB
	for (uint8_t i = 0; i < u8Length; i++)
	{
		tx_to_buf(pBuf[i]);
		crc_ccitt_update(pBuf[i]);
	}
	uCRC = crc_ccitt_crc();
	tx_to_buf(uCRC & 0xFF);	// little endian
	tx_to_buf(uCRC >> 8);
}

void SendLogHelper(const char *s, char ch)
{
// ASSUMPTION: log message is <= 255 bytes
// (if this isn't true, it should be true)

	uint16_t uCRC = 0;
	unsigned char u = strlen(s) + 1;	// add 1 to take into account ASCII letter in front of message
	unsigned char i = 0;

	crc_ccitt_init();

	tx_to_buf(0);	// sync byte
	tx_to_buf(u);	// length LSB
	tx_to_buf(u ^ 255);	// length XOR
	tx_to_buf(ch);
	crc_ccitt_update(ch);
	while (s[i] != 0)
	{
		tx_to_buf(s[i]);
		crc_ccitt_update(s[i]);
		i++;
	}

	uCRC = crc_ccitt_crc();
	tx_to_buf(uCRC & 0xFF);	// little endian
	tx_to_buf(uCRC >> 8);

}

void SendPageRequest(int16_t i16Packet)
{
	unsigned char buf[5] = { 'Z', 00 /* version */, BITS_PER_PAGE, i16Packet & 0xFF, i16Packet >> 8 };
	SendBuf(buf, sizeof(buf));
	g_u8LastPageRequested = i16Packet;

	// reset timeout timer so we know when to timeout
	TCNT1 = 0;
	TIFR1 = (1 << OCF1A);
}

void SendLog(const char *s)
{
	SendLogHelper(s, 'L');
}

void SendPageReRequest()
{
	SendPageRequest(g_u8LastPageRequested);	// request last packet
}

void RequestNextPage()
{
	SendPageRequest(g_u8NextPageToRequest++);
	g_u8PageReRequestCount = 0;
}
