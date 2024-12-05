#include <avr/pgmspace.h>
#include "serial.h"
#include <string.h>
#include "settings.h"
#include "crc.h"
#include "protocol.h"
#include "ldp1000_callbacks.h"	// for ldp1000_send_text_overlay_packet
#include "strings.h"
#include "vbi_inject.h"
#include "vldp-avr.h"
#include "common-ldp.h"

static unsigned char g_bGotSyncByte = 0;
static uint16_t g_u16RawIdx = 0;
static uint16_t g_u16PacketLength = 0;
static uint8_t g_u8PacketXorTmp = 0;
static uint16_t g_u16PacketIdx = 0;
static uint8_t g_pPacketBuf[255];	// TODO : increase this depending on how big our packet can be
static uint8_t g_u8CRCIdx = 0;
static uint8_t g_crc[2];
static ProtocolDiscSwitchStatus_t g_disc_switch_status = PROTOCOL_DISC_SWITCH_IDLE;
static uint8_t g_disc_switch_id = 0;	// which disc id was requested for the disc switch

#ifdef PROTOCOL_CHECK_CRITICAL_SECTION
// used to assert that code isn't doing stuff during critical section that it must not do
uint8_t g_u8InCriticalSection = 0;
#endif // critical section check

#define PROTOCOL_VERSION 0x13

////////////////////////////////

// This function is called while we are waiting for the next vsync so it should only do minimal processing
//  then return.
// Processing mainly should include serial port I/O (both sending and receiving).
void io_think()
{
	// if no character is waiting in the receive buffer, we're done
	if (!rx_is_char_waiting())
	{
#ifdef TX_USE_INT
#else
		tx_think();
#endif // TX_USE_INT
		return;
	}

	// handle 1 byte from receive buffer
	unsigned char ch = rx_from_buf();

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
			log_string(STRING_LENGTH_MISMATCH);
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
		log_string(STRING_CRC_ERROR);
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
			string_to_buf(s, STRING_UNKNOWN_PACKET);
			msg[0] = chCmd;
			msg[1] = 0;
			strcat(s, msg);
			LOG_ERR(s);
		}
		break;
	case 'A':	// set active disc id
		SetActiveDiscIdMemory(g_pPacketBuf[1]);
		break;
	case 'D':	// disc switch acknowledge

		// if we are in the middle of a disc switch
		if (g_disc_switch_status == PROTOCOL_DISC_SWITCH_ACTIVE)
		{
			uint8_t u8MediaServerDiscId = g_pPacketBuf[1];

			if (u8MediaServerDiscId == g_disc_switch_id)
			{
				// if disc switch succeeded
				if (g_pPacketBuf[2] == 1)
				{
					g_disc_switch_status = PROTOCOL_DISC_SWITCH_SUCCESS;
				}
				// else disc switch failed
				else
				{
					g_disc_switch_status = PROTOCOL_DISC_SWITCH_ERROR;
				}
			}
			// else the disc id does not match what we are expecting, so stay in the active state (which will cause us to resend the request)
			else
			{
				// this should never happen, so log it if it does
				log_string(STRING_ERROR);
			}
		}
		// else we can safely ignore this

		break;
	case 'H':	// hello (new settings)
		// check to make sure the version of these settings matches ours
		// (+2 to skip leading 'H' and EEPROM write preference)
		if (*(g_pPacketBuf + 2 + SETTINGS_BYTES_BEFORE_VBI_PATTERN) == 0)
		{
			// save our new settings to RAM (and maybe EEPROM)
			// (+2 to skip the leading 'H' character and eeprom save preference)
			apply_new_settings(g_pPacketBuf[1], g_pPacketBuf + 2, g_u16PacketLength - 2);

			// If disc switching has been used at least once, then always disable disc spin-up delay
			// This is because disc switching is supposed to mimic a frame seek.
			// Also, we should not force caller to know that we need to explicitly disable disc spin-up when doing disc switching. (hide implementation)
			if (g_disc_switch_id != 0)
			{
				common_enable_spinup_delay(0);
			}

			// To be safe, we generally want to reset the active player when we get new settings in
			//  (because the new settings can and do change the active disc)
			// However, we only want to reset the active player if we are not in the middle of a disc switch
			// (If we reset laserdisc interpreter in the middle of a disc switch, then our state will be out of sync)
			if (g_disc_switch_status == PROTOCOL_DISC_SWITCH_IDLE)
			{
				g_bRestartPlayer++;
			}
		}
		else
		{
			LOG_ERR("Version mismatch");
		}
		break;
	case 'M':	// mode change acknowledge
		// TODO : track that media server has received this
		break;
	case 'N':	// request our build number
		MediaServerSendBuildNumber();
		break;
	case 'n':	// request AUX AVR's build number
		VbiInjectSendBuildNumberRequest();
		break;
	case 'O':	// update of next field with a stop code

		// if current ldp mode honors stop codes
		if (GetHonorStopCodesMemory())
		{
			uint32_t u32NextField = g_pPacketBuf[1] | (((uint32_t) g_pPacketBuf[2]) << 8) | (((uint32_t) g_pPacketBuf[3]) << 16) | (((uint32_t) g_pPacketBuf[4]) << 24);
			ldpc_set_next_field_with_stopcode(u32NextField);
		}
		break;
	case 'p':	// firmware update page
		VbiInjectSendFirmwarePage(g_pPacketBuf, g_u16PacketLength);
		break;
	case 'S':	// request current settings
		MediaServerSendSettings();
		break;
	case 'T':	// request most recent text overlay information again
		ldp1000_send_text_overlay_packet();
		break;
	case 'Z':	// force reprogram
		g_bRestartPlayer++;
		g_bReprogramForced++;
		break;
	case 'z':	// force AUX reprogram
		VbiInjectSendFirmwareForceUpdate();
		break;
		
	}

}

///////////////////////////////////////////

void MediaServerSendSmallBuf(unsigned char *pBuf, unsigned char u8Length)
{
	unsigned int uCRC = 0;

#ifdef PROTOCOL_CHECK_CRITICAL_SECTION
	// we cannot call tx_to_buf while in a critical section because tx_to_buf enables TX interrupt
	if (g_u8InCriticalSection)
	{
		MediaServerSendFatal();
	}
#endif

	crc_ccitt_init();

	tx_to_buf(0);	// sync byte
	tx_to_buf(u8Length);	// length LSB
	tx_to_buf(u8Length ^ 255);	// xor of length
	for (unsigned char i = 0; i < u8Length; i++)
	{
		tx_to_buf(pBuf[i]);
		crc_ccitt_update(pBuf[i]);
	}
	uCRC = crc_ccitt_crc();
	tx_to_buf(uCRC & 0xFF);	// little endian
	tx_to_buf(uCRC >> 8);
}

void MediaServerSendBlankScreen()
{
	unsigned char buf = 'B';
	MediaServerSendSmallBuf(&buf, 1);
}

void MediaServerSendDiscSwitch(uint8_t u8DiscId)
{
	unsigned char buf[2] = { 'D', u8DiscId };
	MediaServerSendSmallBuf(buf, sizeof(buf));
}

void MediaServerSendLogHelper(const char *s, char ch)
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

void MediaServerSendError(const char *s)
{
#ifdef PROTOCOL_CHECK_CRITICAL_SECTION
	// we cannot call tx_to_buf while in a critical section because tx_to_buf enables TX interrupt
	if (g_u8InCriticalSection)
	{
		MediaServerSendFatal();
	}
#endif

	MediaServerSendLogHelper(s, 'E');
}

void MediaServerSendField(uint32_t u, LDPCAudioStatus_t u8)
{
	unsigned char buf[5];
	buf[0] = 'F';
	buf[1] = (u & 0xFF);
	buf[2] = ((u >> 8) & 0xFF);
	buf[3] = ((u >> 16) & 0xFF);
	buf[4] = ((u >> 24) | (u8 << 6));
	MediaServerSendSmallBuf(buf, sizeof(buf));
}

void MediaServerSendFieldWithText(uint32_t u, uint8_t u8ID, LDPCAudioStatus_t u8)
{
	unsigned char buf[6];
	buf[0] = 'G';
	buf[1] = (u & 0xFF);
	buf[2] = ((u >> 8) & 0xFF);
	buf[3] = ((u >> 16) & 0xFF);
	buf[4] = ((u >> 24) | (u8 << 6));
	buf[5] = u8ID;
	MediaServerSendSmallBuf(buf, sizeof(buf));
}

void MediaServerSendHello(uint32_t u)
{
	unsigned char buf[1] = { 'H' };
	MediaServerSendSmallBuf(buf, sizeof(buf));
}

void MediaServerSendLog(const char *s)
{
#ifdef PROTOCOL_CHECK_CRITICAL_SECTION
	// we cannot call tx_to_buf while in a critical section because tx_to_buf enables TX interrupt
	if (g_u8InCriticalSection)
	{
		MediaServerSendFatal();
	}
#endif

	MediaServerSendLogHelper(s, 'L');
}

void MediaServerSendBuildNumber()
{
	uint8_t buf[5] = { 'N', 0, 0, 0, 0 };

	// grab CRC value from end of ROM (updater put it there for us)
	uint16_t u16 = pgm_read_word(FLASH_CRC_OFFSET);
	buf[1] = u16;
	buf[2] = u16 >> 8;

	MediaServerSendSmallBuf(buf, sizeof(buf));
}

void MediaServerSendSettings()
{
	unsigned char buf[] = { 'S',
		PROTOCOL_VERSION,	// version of the protocol
		0, 0,	// persistent id
		0,	// diagnose
		0,	// LDP MODE
		0,	// auto detection
		0,	// active disc id
		0, 0	// dynamic id
		};

	uint16_t u16PersID = GetPersistentIdentifierEeprom();
	uint16_t u16DynID = GetDynamicIdentifierEeprom();

	// little-endian formatted
	buf[2] = u16PersID & 0xFF;
	buf[3] = u16PersID >> 8;

	// diagnose
	buf[4] = (uint8_t) IsDiagnosticsEnabledEeprom();
	
	// LDP mode
	// intentional choice to return the active player to hide our internal implementation
	// If there is no active player yet, we are okay with returning "none"
	buf[5] = (uint8_t) GetActiveLDPType();

	// auto detection
	buf[6] = (uint8_t) IsAutoDetectionEnabledEeprom();

	// active disc id
	buf[7] = (uint8_t) GetActiveDiscIdMemory();

	// little-endian formatted
	buf[8] = u16DynID & 0xFF;
	buf[9] = u16DynID >> 8;

	MediaServerSendSmallBuf(buf, sizeof(buf));
}

void MediaServerSendRxLog(uint8_t u8)
{
	uint8_t buf[2] = { 'X', u8 };

#ifdef PROTOCOL_CHECK_CRITICAL_SECTION
	// we cannot call tx_to_buf while in a critical section because tx_to_buf enables TX interrupt
	if (g_u8InCriticalSection)
	{
		MediaServerSendFatal();
	}
#endif

	MediaServerSendSmallBuf(buf, sizeof(buf));

}

void MediaServerSendTxLog(uint8_t u8)
{
	uint8_t buf[2] = { 'Y', u8 };

#ifdef PROTOCOL_CHECK_CRITICAL_SECTION
	// we cannot call tx_to_buf while in a critical section because tx_to_buf enables TX interrupt
	if (g_u8InCriticalSection)
	{
		MediaServerSendFatal();
	}
#endif

	MediaServerSendSmallBuf(buf, sizeof(buf));

}

void MediaServerSendAuxLogMessage(uint8_t *pLogMsgPacket, uint8_t u8Length)
{
	pLogMsgPacket[0] = 'l';	// change L to l to indicate that it comes from the AUX AVR
	
	MediaServerSendSmallBuf(pLogMsgPacket, u8Length);
}

void MediaServerSendAuxBuildNumber(uint8_t *pBuildNumPacket, uint8_t u8Length)
{
	pBuildNumPacket[0] = 'n';	// change to n to indicate that it comes from the AUX AVR
	
	MediaServerSendSmallBuf(pBuildNumPacket, u8Length);
}

void MediaServerSendAuxPageRequest(uint8_t *pPageReqPacket, uint8_t u8Length)
{
	pPageReqPacket[0] = 'z';	// change to z to indicate that it comes from the AUX AVR
	
	MediaServerSendSmallBuf(pPageReqPacket, u8Length);
}

void protocol_initiate_disc_switch(uint8_t idDisc)
{
	// want screen to go blank as soon as disc switch starts
	MediaServerSendBlankScreen();

	// stop the disc from playing so that we don't see 'garbage' fields during the transition
	ldpc_stop();	
	g_disc_switch_status = PROTOCOL_DISC_SWITCH_ACTIVE;
	g_disc_switch_id = idDisc;
}

ProtocolDiscSwitchStatus_t protocol_get_disc_switch_status_and_think()
{
	ProtocolDiscSwitchStatus_t res = g_disc_switch_status;

	switch (g_disc_switch_status)
	{
	default:
	case PROTOCOL_DISC_SWITCH_IDLE:
		break;
	case PROTOCOL_DISC_SWITCH_ACTIVE:
		// if we are active, send (another) disc switch request.  these requests are idempotent, so no problem sending duplicates.
		MediaServerSendBlankScreen();	// in unlikely event that previous blank screen request was lost, keep resending it
		MediaServerSendDiscSwitch(g_disc_switch_id);
		break;
	case PROTOCOL_DISC_SWITCH_SUCCESS:
	case PROTOCOL_DISC_SWITCH_ERROR:
		g_disc_switch_status = PROTOCOL_DISC_SWITCH_IDLE;
		break;
	}

	return res;
}

#ifdef PROTOCOL_CHECK_CRITICAL_SECTION
void MediaServerSendFatal()
{
	char s[30];
	string_to_buf(s, STRING_CRITICAL_SECTION_VIOLATION);
	MediaServerSendLogHelper(s, 'E');

	// lockup forever to ensure developer notices this
//	for (;;)
	{
	}
}
#endif
