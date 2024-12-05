#ifndef VBI_INJECT_H
#define VBI_INJECT_H

#include <stdint.h>

#include "serial2.h"

// we've improved our serial2 code enough that we just use it directly
#define vbi_inject_tx(u8) tx2_to_buf(u8)
#define vbi_inject_tx_enable(u8Enabled) tx2_enable(u8Enabled)

void vbi_inject_init();
void vbi_inject_shutdown();
uint8_t is_vbi_inject_active();
void vbi_inject_think();
void ProcessVbiInjectPacket();
void vbi_inject_tx(uint8_t u8);
void vbi_inject_tx_enable(uint8_t u8Enabled);
void VbiInjectSendBuf(unsigned char *pBuf, uint16_t u16Length);
void VbiInjectSendVideoMute(uint8_t bStandByEnabled);
void VbiInjectSendBuildNumberRequest();
void VbiInjectSendFirmwarePage(uint8_t *pPagePacket, uint16_t u16Length);
void VbiInjectSendVbiUpdate(uint8_t u8FieldFlag, uint32_t pu32Lines[]);
void VbiInjectSendFirmwareForceUpdate();

#endif // VBI_INJECT_H
