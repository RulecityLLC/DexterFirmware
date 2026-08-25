#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <ldp-abst/ldpc.h>
#include "serial.h"

/*
These methods implement the I/O protocol documented at https://www.daphne-emu.com:9443/mediawiki/index.php/VLDP-HW#Serial_Communications

In case that URL becomes unavailable, here is a brief description of the design:

- Small packets are sent with a simple header and a CRC-16 footer.
- All packets are not guaranteed to be delivered, and are idempotent.
- If a packet fails integrity check (CRC check or length check), it is dropped by the receiver.  The sender usually does not know (or care) that the packet it sent was dropped.
- The 'media server' in this case is the Raspberry Pi 2 (or other PC-type device) that streams video/audio.
    The media server is only connected to the main AVR's serial lines.  To communicate with the aux AVR, it sends packets to the main AVR which then delivers them to the aux AVR (and vice versa).
- The AVR's TX/RX buffers are not that big so they can be easily overflowed if the sender spams too much data.
    Received packets are processed automatically by an ISR.  But nothing will stop you from overflowing the TX buffer by sending too much.  Therefore, keep logging to a minimum.

Intent:
The reason that packets are unreliable (no retry) is because tests have shown that almost all packets are delivered properly anyway, and the current design improves performance and simplifies the implementation.
The reason for the CRC check: Early in the design, one collaborator suggested that we didn't need a CRC check at all but I'm really glad we added one.
    The CRC check makes it easy to detect corrupt packets and drop them.  And corrupt packets will occur if the TX/RX buffers overflow (or if there's some other logic error in the source code).
    Since the Dexter firmware is updated via this protocol mechanism, and since sending big packets to the aux AVR often results in CRC errors (see vbi_inject.h for explanation), it's especially
      important to have a CRC check to prevent corrupt program code being stored to Dexter's flash memory.

*/

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

#endif // PROTOCOL_H
