#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "serial_packet.h"
#include "strings.h"
#include <stdint.h>

/*

See MainAVR/src/vbi_inject.h for discussion about why 16 MHz crystal was chosen for this aux AVR, and why this introduces occasional I/O errors.

*/

void ProtocolSetup();
void io_think();
void OnSerialPacketError(SerialPacketErrCode_t code, uint16_t u16Val1, uint16_t u16Val2);
void ProcessPacket();
void ForceReprogram();
void LogString(StringID id);
void SendBuf(unsigned char *pBuf, uint8_t u8Length);
void SendLogHelper(const char *s, char ch);
void SendLog(const char *s);
void SendBuildNumber();

#endif // PROCOTOL_H
