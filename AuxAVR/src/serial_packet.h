#ifndef SERIAL_PACKET_H
#define SERIAL_PACKET_H

#ifdef __cplusplus
extern "C"
{
#endif // C++

#include "datatypes.h"

#define MAX_PACKET_SIZE 11

// so that we can have more than one instance of serial packet if we really want to
typedef struct
{
	uint8_t bGotSyncByte;
	uint16_t u16RawIdx;
	uint16_t u16PacketLength;
	uint16_t u16PacketIdx;
	uint8_t u8CRCIdx;
	uint16_t crc_calculated;
	uint8_t crc_received[2];
	uint8_t prefixBuf[4];
	uint8_t pPacketBuf[MAX_PACKET_SIZE];
} SerialPacketContext_t;

// gets called when a good packet has been decoded and is ready to be processed.
// The packet contents should be retrieved shortly after this call, before the next byte is processed.
extern void (*g_serpck_on_good_packet)();

typedef enum
{
	SERPCK_ERR_PACKET_TOO_BIG,
	SERPCK_ERR_BAD_XOR,
	SERPCK_ERR_BAD_CRC
} SerialPacketErrCode_t;

extern void (*g_serpck_error)(SerialPacketErrCode_t code, uint16_t u16Val1, uint16_t u16Val2);

// so that we can have more than one instance of serial packet if we really want to
// (caller can read/write context memory after retrieving pointer)
SerialPacketContext_t *serpck_get_context();

// resets context to default values
void serpck_reset_context();

// when byte comes in via serial interface, call this method to process
void serpck_process_byte(uint8_t ch);

#ifdef __cplusplus
}
#endif // C++

#endif // SERIAL_PACKET_H
