#ifndef IO_H
#define IO_H

#include <ldp-abst/ldpc.h>
#include "serial.h"

typedef enum
{
	PROTOCOL_DISC_SWITCH_IDLE,
	PROTOCOL_DISC_SWITCH_ACTIVE,
	PROTOCOL_DISC_SWITCH_SUCCESS,
	PROTOCOL_DISC_SWITCH_ERROR
} ProtocolDiscSwitchStatus_t;

void io_think();
void ProcessPacket();
void MediaServerSendSmallBuf(unsigned char *pBuf, unsigned char u8Length);
void MediaServerSendBlankScreen();
void MediaServerSendDiscSwitch(uint8_t u8DiscId);
void MediaServerSendError(const char *s);
void MediaServerSendField(uint32_t u, LDPCAudioStatus_t u8);
void MediaServerSendFieldWithText(uint32_t u, uint8_t u8ID, LDPCAudioStatus_t u8);
void MediaServerSendHello();
void MediaServerSendLog(const char *s);
void MediaServerSendBuildNumber();
void MediaServerSendSettings();
void MediaServerSendRxLog(uint8_t u8);
void MediaServerSendTxLog(uint8_t u8);

// AUX pass throughs
void MediaServerSendAuxLogMessage(uint8_t *pLogMsgPacket, uint8_t u8Length);
void MediaServerSendAuxBuildNumber(uint8_t *pBuildNumPacket, uint8_t u8Length);
void MediaServerSendAuxPageRequest(uint8_t *pPageReqPacket, uint8_t u8Length);

void protocol_initiate_disc_switch(uint8_t idDisc);

// this method does more than one thing because of performance concerns
ProtocolDiscSwitchStatus_t protocol_get_disc_switch_status_and_think();

#define LOG(s)  MediaServerSendLog(s)
#define LOG_ERR(s)	MediaServerSendError(s)

// uncomment this for development work, comment out for production once code is solid
#define PROTOCOL_CHECK_CRITICAL_SECTION

#ifdef PROTOCOL_CHECK_CRITICAL_SECTION
// debug version (safety checking)

void MediaServerSendFatal();
extern uint8_t g_u8InCriticalSection;
#define ENTER_CRITICAL_SECTION() TX_INT_DISABLE(); g_u8InCriticalSection++
#define LEAVE_CRITICAL_SECTION() g_u8InCriticalSection = 0; TX_INT_ENABLE()
#else
// production version (no safety checking)
#define ENTER_CRITICAL_SECTION() TX_INT_DISABLE()
#define LEAVE_CRITICAL_SECTION() TX_INT_ENABLE()
#endif

#endif // IO_H