#include <stdio.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "serial.h"
#include <string.h>
#include "crc.h"
#include "serial_packet.h"
#include "protocol.h"
#include "vbi_inject.h"
#include "build.h"

// 328p
#define EEPROM_SIZE_BYTES 1024

extern uint8_t g_u8WhiteFlags[2];
extern uint32_t g_u32PicNums[3][2];
extern uint8_t g_bUnmuteQueued;

// the way we hook into serial_packet
extern SerialPacketContext_t g_ctx;

////////////////////////////////

void ProtocolSetup()
{
	g_serpck_on_good_packet = ProcessPacket;
	g_serpck_error = OnSerialPacketError;
	serpck_reset_context();
}

// call this regularly
void io_think()
{
	// if no character is waiting in the receive buffer, we're done
	if (!rx_is_char_waiting())
	{
		// empty output buffer
		tx_think();
		return;
	}

	// handle 1 byte from receive buffer
	uint8_t ch = rx_from_buf();

	serpck_process_byte(ch);
}

void OnSerialPacketError(SerialPacketErrCode_t code, uint16_t u16Val1, uint16_t u16Val2)
{
	char s[50];	// bigger than it needs to be for convenience, we aren't having memory issues yet
	char s1[60];

	switch (code)
	{
	case SERPCK_ERR_PACKET_TOO_BIG:
		string_to_buf(s, STRING_PACKET_TOO_BIG);
		snprintf(s1, sizeof(s1), s, u16Val1);
		break;
	case SERPCK_ERR_BAD_XOR:
		string_to_buf(s, STRING_BAD_PACKET_XOR);
		snprintf(s1, sizeof(s1), s, u16Val1, u16Val2);
		break;
	case SERPCK_ERR_BAD_CRC:
		string_to_buf(s, STRING_CRC_ERROR);
		snprintf(s1, sizeof(s1), s, u16Val1, u16Val2);
		break;
	default:
		string_to_buf(s, STRING_UNKNOWN_ERROR_ENUM);
		snprintf(s1, sizeof(s1), s, code);
		break;
	}

	SendLog(s1);
}

void ProcessPacket()
{
	// for convenience
	uint8_t *pPacketBuf = g_ctx.pPacketBuf;
	uint16_t u16PacketLength = g_ctx.u16PacketLength;

	// get command code
	unsigned char chCmd = pPacketBuf[0];

	switch (chCmd)
	{
	default:
		{
			char s[30];
			char s1[30];
			string_to_buf(s, STRING_UNKNOWN_PACKET);
			snprintf(s1, sizeof(s1), s, chCmd);
			SendLog(s1);
		}
		break;
	case 'M':	// mute video signal

		// clear our last picture number info so that when we come back from mute status we won't render it (not sure if this ever could happen)
		g_u8WhiteFlags[0] = g_u8WhiteFlags[0] = 0;
		g_u32PicNums[0][0] = 0;
		g_u32PicNums[0][1] = 0;
		g_u32PicNums[1][0] = 0;
		g_u32PicNums[1][1] = 0;
		g_u32PicNums[2][0] = 0;
		g_u32PicNums[2][1] = 0;

		ENABLE_VIDEO_MUTE();

		// if sender is providing STAND BY command also
		if (u16PacketLength == 2)
		{
			// if we are supposed to turn STAND BY line on
			if (pPacketBuf[1] != 0)
			{
				ENABLE_PR8210A_STAND_BY();
			}
			else
			{
				DISABLE_PR8210A_STAND_BY();
			}
		}

		break;
	case 'N':	// build number request
		SendBuildNumber();
		break;
	case 'V':	// incoming VBI
		{
			uint8_t u8WhichField = (pPacketBuf[1] >> 1) & 1;	// bit 1 is top/bottom field bit
			g_u8WhiteFlags[u8WhichField] = pPacketBuf[1] & 1;	// bit 0 is the white flag value

			// line 16
			g_u32PicNums[0][u8WhichField] = ((uint32_t) pPacketBuf[2] << 16) |
				((uint16_t) pPacketBuf[3] << 8) | (pPacketBuf[4]);

			// line 17
			g_u32PicNums[1][u8WhichField] = ((uint32_t)pPacketBuf[5] << 16) |
				((uint16_t)pPacketBuf[6] << 8) | (pPacketBuf[7]);

			// line 18
			g_u32PicNums[2][u8WhichField] = ((uint32_t)pPacketBuf[8] << 16) |
				((uint16_t)pPacketBuf[9] << 8) | (pPacketBuf[10]);
		}

		// Sending vbi data implicitly unmutes video signal.
		// This side effect is necessary to protect against dropped packets.
		g_bUnmuteQueued = 1;
		DISABLE_PR8210A_STAND_BY();
		break;
	case 'Z':	// force firmware update
		ForceReprogram();
		break;
	}
}

void ForceReprogram()
{
	// tell bootloader that it needs to reprogram
	eeprom_update_word((uint16_t *) EEPROM_SIZE_BYTES - 2, 0);

	// disable all interrupts so bootloader runs properly
	EIMSK = 0;	// disable all external interrupts
	SERIAL_INT_DISABLE();	// disable serial port interrupt
	cli();	// disable all interrupts

	// jump to the bootloader now
	//asm volatile("jmp 0x3C00"::);
	asm volatile("jmp 0x7800"::);	// for some reason the target address must be doubled for the compiler to translate it correctly
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

void SendBuildNumber()
{
	unsigned char buf[5] = { 'N', 0, 0, BUILD_NUMBER, BUILD_NUMBER >> 8 };
	
	// grab CRC value from end of ROM (updater put it there for us)
	uint16_t u16 = pgm_read_word(FLASH_CRC_OFFSET);
	buf[1] = u16;
	buf[2] = u16 >> 8;
	
	SendBuf(buf, sizeof(buf));
}

void SendLog(const char *s)
{
	SendLogHelper(s, 'L');
}
