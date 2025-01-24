#include "serial2.h"
#include <string.h>
#include "crc.h"
#include "vbi_inject.h"
#include "dexter_strings.h"
#include "protocol.h"
#include "vldp-avr.h"
#include "timer-global.h"

static uint8_t g_bVbiInjectActive = 0;
static uint8_t g_bGotSyncByte = 0;
static uint8_t g_u8RawIdx = 0;
static uint8_t g_u8PacketLength = 0;
static uint8_t g_u8PacketIdx = 0;
static uint8_t g_pPacketBuf[20];	// should be really small, but big enough to handle short log messages
static uint8_t g_u8CRCIdx = 0;
static uint8_t g_crc[2];

// it's important that we don't send non-bootloader commands if the bootloader is trying to program new firmware
static uint8_t g_u8BootloaderIsRunning = 0;

// slow global timer indicating when we last saw the bootloader say something (so that we can timeout if the bootloader isn't running anymore)
static uint8_t g_u8BootloaderLastSeenTime = 0;

////////////////////////////////

void vbi_inject_reset()
{
	g_bGotSyncByte = 0;
	g_u8PacketLength = 0;
	g_u8PacketIdx = 0;
	g_u8CRCIdx = 0;
	g_u8RawIdx = 0;
}

void vbi_inject_init()
{
	vbi_inject_reset();

	// the AUX AVR is 16 mhz which runs better at 76800 than 115200
	serial2_init(76800);

	g_bVbiInjectActive = 1;
}

void vbi_inject_shutdown()
{
	serial2_shutdown();

	g_bVbiInjectActive = 0;
}

uint8_t is_vbi_inject_active()
{
	return g_bVbiInjectActive;
}

void vbi_inject_think()
{
	if (g_u8BootloaderIsRunning)
	{
		// If we haven't seen the bootloader is quite a while, then assume that the bootloader isn't running

		// 18,432,000 / 1024 is 18,000 cycles (1 second)
		// So four seconds would be 72,000 cycles (0x11940)
		// Divided by 256 (the range of the regular global timer) this becomes about 0x119 slow timer ticks)
		if (GLOBAL_SLOW_TIMER_DIFF_SINCE_START(g_u8BootloaderLastSeenTime) > 0x119)
		{
			g_u8BootloaderIsRunning = 0;
		}
	}

	// if no character is waiting in the receive buffer, we're done
	if (!rx2_is_char_waiting())
	{
		return;
	}

	// handle 1 byte from receive buffer
	unsigned char ch = rx2_from_buf();

	// if we are waiting for the sync byte
	if (!g_bGotSyncByte)
	{
		// If we get a 0, it means we've found the beginning of a new packet
		//  so we are _probably_ in sync (CRC could contain a 0 for example, so this is not guaranteed)
		// If we aren't in sync, this will definitely help us get in sync until we eventually are in sync again.
		if (ch != 0)
		{
			g_u8RawIdx = 0;
			return;	// don't increment RawIdx
		}

		g_bGotSyncByte = 1;
	}
	// else if we're waiting for the first byte of the packet length
	else if (g_u8RawIdx == 1)
	{
		g_u8PacketLength = ch;
	}
	// else if we're waiting for XOR byte of the packet length
	else if (g_u8RawIdx == 2)
	{
		// the second byte MUST be the XOR of the first byte (so we know the packet length is correct)
		if (g_u8PacketLength == (ch ^ 0xFF))
		{
			if (g_u8PacketLength < sizeof(g_pPacketBuf))
			{
				// everything is ok
			}
			else
			{
				log_string(STRING_AUX_PACKET_TOO_BIG);
				goto reset;
			}
		}
		else
		{
			log_string(STRING_AUX_LENGTH_MISMATCH);
			goto reset;
		}
	}
	// else if we are in the middle of receiving the packet
	else if (g_u8PacketIdx < g_u8PacketLength)
	{
		g_pPacketBuf[g_u8PacketIdx++] = ch;
	}
	// else if we are receiving the CRC at the end of the packet
	else if (g_u8CRCIdx < 2)
	{
		g_crc[g_u8CRCIdx++] = ch;

		// if we've received the full packet, then process it
		if (g_u8CRCIdx == 2)
		{
			ProcessVbiInjectPacket();

reset:
			vbi_inject_reset();
			return;	// don't increment rawIdx
		}
	}

	// this makes it convenient to figure out where we are
	g_u8RawIdx++;

}

void ProcessVbiInjectPacket()
{
	// check CRC first
	crc_ccitt_init();
	for (uint8_t i = 0; i < g_u8PacketLength; i++)
	{
		crc_ccitt_update(g_pPacketBuf[i]);
	}

	uint16_t u16ActualCRC = crc_ccitt_crc();
	uint16_t u16ReceivedCRC = g_crc[0] | (g_crc[1] << 8);	// little endian

	// if CRC does not match
	if (u16ReceivedCRC != u16ActualCRC)
	{
		log_string(STRING_AUX_CRC_ERROR);
		return;
	}

	// get command code
	unsigned char chCmd = g_pPacketBuf[0];

	switch (chCmd)
	{
	default:
		{
			char s[20];
			char msg[2];
			string_to_buf(s, STRING_AUX_UNKNOWN_PACKET);
			msg[0] = chCmd;
			msg[1] = 0;
			strcat(s, msg);
			LOG_ERR(s);
		}
		break;
	case 'L':	// AUX AVR sends log message
		MediaServerSendAuxLogMessage(g_pPacketBuf, g_u8PacketLength);	// pass thru
		break;
	case 'N':	// AUX AVR sends build number
		MediaServerSendAuxBuildNumber(g_pPacketBuf, g_u8PacketLength);	// pass thru
		break;
	case 'Z':	// AUX AVR requests firmware page
		MediaServerSendAuxPageRequest(g_pPacketBuf, g_u8PacketLength);	// pass thru

		// this is a bootloader message which means that the regular firmware isn't running
		g_u8BootloaderIsRunning = 1;
		g_u8BootloaderLastSeenTime = GET_GLOBAL_SLOW_TIMER_VALUE();
		break;		
	}

}

///////////////////////////////////////////

void VbiInjectSendBuf(unsigned char *pBuf, uint16_t u16Length)
{
	// safety check: don't use serial port if vbi inject is not active to avoid stomping on serial LDP drivers
	// (protocol.c stuff will not check to see if we are active or not so we have to do the check)
	if (!g_bVbiInjectActive)
	{
		return;
	}

	unsigned int uCRC = 0;

	crc_ccitt_init();

	vbi_inject_tx(0);	// sync byte
	vbi_inject_tx(u16Length);	// length LSB
	vbi_inject_tx(u16Length >> 8);	// length MSB
	vbi_inject_tx(u16Length ^ 255);	// xor of length
	vbi_inject_tx((u16Length >> 8) ^ 255);
	for (uint16_t i = 0; i < u16Length; i++)
	{
		vbi_inject_tx(pBuf[i]);
		crc_ccitt_update(pBuf[i]);
	}
	uCRC = crc_ccitt_crc();
	vbi_inject_tx(uCRC);	// little endian
	vbi_inject_tx(uCRC >> 8);
}

void VbiInjectSendVideoMute(uint8_t bStandByEnabled)
{
	// if bootloader is running, suppress all video mute commands because they interfere with firmware updates
	if (g_u8BootloaderIsRunning)
	{
		return;
	}

	unsigned char buf[2] = { 'M', bStandByEnabled };
	VbiInjectSendBuf(buf, sizeof(buf));
}

void VbiInjectSendBuildNumberRequest()
{
	// NOTE: we must currently always send this request because the only
	//  way we can detect whether the firmware is running is if we get
	//  a proper response from this request.

	// if we are running
	if (g_bVbiInjectActive)
	{
		// NOTE: this is pass-through to aux avr
		unsigned char buf[1] = { 'N' };
		VbiInjectSendBuf(buf, sizeof(buf));
	}
	// else respond with a -1 to indicate we are not running
	else
	{
		unsigned char buf[5] = { 'N', 0xFF, 0xFF, 0xFF, 0xFF };
		MediaServerSendAuxBuildNumber(buf, sizeof(buf));
	}
}

void VbiInjectSendFirmwarePage(uint8_t *pPagePacket, uint16_t u16Length)
{
	pPagePacket[0] = 'P';	// convert 'p' to 'P', this is coupled to the protocol.c stuff to avoid using a lot of memory and extra code
	VbiInjectSendBuf(pPagePacket, u16Length);
}

void VbiInjectSendVbiUpdate(uint8_t u8FieldFlag, uint32_t pu32Lines[])
{
	uint8_t buf[11] = {
		'V', u8FieldFlag,
		pu32Lines[0] >> 16, pu32Lines[0] >> 8, pu32Lines[0],
		pu32Lines[1] >> 16, pu32Lines[1] >> 8, pu32Lines[1],
		pu32Lines[2] >> 16, pu32Lines[2] >> 8, pu32Lines[2],
		 };

	// if bootloader is running, suppress all VBI update commands because they interfere with firmware updates
	if (g_u8BootloaderIsRunning)
	{
		return;
	}

	VbiInjectSendBuf(buf, sizeof(buf));
}

void VbiInjectSendFirmwareForceUpdate()
{
	unsigned char buf[1] = { 'Z' };
	VbiInjectSendBuf(buf, sizeof(buf));
}
