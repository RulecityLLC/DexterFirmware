#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "strings.h"
#include <stdint.h>

void io_think();
void ProcessPacket();
void LogString(StringID id);
void SendBuf(unsigned char *pBuf, uint8_t u8Length);
void SendLogHelper(const char *s, char ch);
void SendPageRequest(int16_t i16Packet);
void SendLog(const char *s);
void SendPageReRequest();
void RequestNextPage();

#endif // PROCOTOL_H
