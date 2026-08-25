#ifndef VBI_INJECT_H
#define VBI_INJECT_H

#include <stdint.h>

#include "serial2.h"

/*
vbi_inject.* handles the I/O between the main AVR and the aux AVR.

The main AVR communicates to the aux AVR at 76800 bps.  However, the aux AVR has a 16 MHz crystal, so it cannot communicate at 76800 bps perfectly.
 This means CRC errors are common if the packet size is larger (ie when firmware is being updated).
To have zero errors, the crystal would need to be 15.9744 MHz.   (12+1)(16*76800) == 15974400
Firmware updates will often have errors that get caught by the CRC checker.  These bad packets just retry and eventually succeed.  This behavior is by design.
During normal VBI streaming operation, errors are rare.

I chose 16 MHz intentionally so that the injected VBI has the proper shape (2 uS per bit cell).  The CRC errors during firmware update are a nuisance worth the cost.

I chose 76800 bps instead of 115200 bps to reduce the number of errors that can occur.  76800 is still fast enough for a regular stream of VBI data during normal operation.

*/

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
