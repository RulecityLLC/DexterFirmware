#include <avr/io.h>

// tests are free to re-assign these at will

ReadWriteRegister<uint8_t> *g_pDDRAPtr;
ReadWriteRegister<uint8_t> *g_pDDRBPtr;
ReadWriteRegister<uint8_t> *g_pDDRCPtr;
ReadOnlyRegister<uint8_t> *g_pPINAPtr;
ReadOnlyRegister<uint8_t> *g_pPINBPtr;
ReadOnlyRegister<uint8_t> *g_pPINCPtr;
ReadWriteRegister<uint8_t> *g_pPORTAPtr;
ReadWriteRegister<uint8_t> *g_pPORTBPtr;
ReadWriteRegister<uint8_t> *g_pPORTCPtr;
ReadWriteRegister<uint8_t> *g_pTCNT0Ptr;
ReadWriteRegister<uint16_t> *g_pTCNT1Ptr;
ReadWriteRegister<uint8_t> *g_pTIFR1Ptr;
ReadWriteRegister<uint8_t> *g_pTIMSK1Ptr;
