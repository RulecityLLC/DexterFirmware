#ifndef IO_H
#define IO_H

#include <cstdint>

///////////////////////////////////////

#define CS11	1

#define INTF2	2

#define OCF1A 1
#define OCIE1A 1

#define PA0	0
#define PA1	1
#define PA2	2
#define PA3	3
#define PA4	4
#define PA5	5
#define PA6	6
#define PA7	7

#define PB6 6

#define PC0 0
#define PC1 1

#define PCIE0	0

#define PCINT0	0

#define WGM12	3

///////////////////////////////////////

template <typename T> class ReadOnlyRegister
{
public:
	virtual ~ReadOnlyRegister() = default;

	virtual operator T() const = 0;
};

///////////////////////////////////////

template <typename T> class ReadWriteRegister : public ReadOnlyRegister<T>
{
public:
	virtual ReadWriteRegister& operator = (T val) = 0;
	virtual ReadWriteRegister& operator &= (T val) = 0;
	virtual ReadWriteRegister& operator |= (T val) = 0;
};

///////////////////////////////////////

extern ReadWriteRegister<uint8_t> *g_pDDRAPtr;
#define DDRA *g_pDDRAPtr

extern ReadWriteRegister<uint8_t> *g_pDDRBPtr;
#define DDRB *g_pDDRBPtr

extern ReadWriteRegister<uint8_t> *g_pDDRCPtr;
#define DDRC *g_pDDRCPtr

extern ReadWriteRegister<uint8_t> *g_pEIFRPtr;
#define EIFR *g_pEIFRPtr

extern ReadWriteRegister<uint8_t> *g_pPCICRPtr;
#define PCICR *g_pPCICRPtr

extern ReadWriteRegister<uint8_t> *g_pPCMSK0Ptr;
#define PCMSK0 *g_pPCMSK0Ptr

extern ReadOnlyRegister<uint8_t> *g_pPINAPtr;
#define PINA *g_pPINAPtr

extern ReadOnlyRegister<uint8_t> *g_pPINBPtr;
#define PINB *g_pPINBPtr

extern ReadOnlyRegister<uint8_t> *g_pPINCPtr;
#define PINC *g_pPINCPtr

extern ReadWriteRegister<uint8_t> *g_pPORTAPtr;
#define PORTA *g_pPORTAPtr

extern ReadWriteRegister<uint8_t> *g_pPORTBPtr;
#define PORTB *g_pPORTBPtr

extern ReadWriteRegister<uint8_t> *g_pPORTCPtr;
#define PORTC *g_pPORTCPtr

extern ReadWriteRegister<uint8_t> *g_pTCCR1BPtr;
#define TCCR1B *g_pTCCR1BPtr

extern ReadWriteRegister<uint8_t> *g_pTCNT0Ptr;
#define TCNT0 *g_pTCNT0Ptr

extern ReadWriteRegister<uint16_t> *g_pTCNT1Ptr;
#define TCNT1 *g_pTCNT1Ptr

extern ReadWriteRegister<uint8_t> *g_pTIFR1Ptr;
#define TIFR1 *g_pTIFR1Ptr

extern ReadWriteRegister<uint8_t> *g_pTIMSK1Ptr;
#define TIMSK1 *g_pTIMSK1Ptr

#endif //IO_H
