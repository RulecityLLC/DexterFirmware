#include <avr/io.h>

// tests are free to re-assign these at will

ReadOnlyRegister<uint8_t> *g_pPINAPtr;
ReadWriteRegister<uint8_t> *g_pPORTAPtr;
ReadWriteRegister<uint8_t> *g_pTCNT0Ptr;
ReadWriteRegister<uint16_t> *g_pTCNT1Ptr;
ReadWriteRegister<uint8_t> *g_pTIFR1Ptr;
ReadWriteRegister<uint8_t> *g_pTIMSK1Ptr;
