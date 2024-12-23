#ifndef IO_H
#define IO_H

#include <cstdint>

///////////////////////////////////////

#define OCF1A 1
#define OCIE1A 1

///////////////////////////////////////

class TCNT1TestInterface
{
public:
	virtual ~TCNT1TestInterface() = default;

	virtual TCNT1TestInterface& operator = (uint16_t index) = 0;
	virtual operator uint16_t() const = 0;
};

extern TCNT1TestInterface *g_pTCNT1Ptr;
#define TCNT1 *g_pTCNT1Ptr

////////////////////////////////////////

class TIFR1TestInterface
{
public:
	virtual ~TIFR1TestInterface() = default;

	virtual TIFR1TestInterface& operator |= (uint8_t index) = 0;
};

extern TIFR1TestInterface *g_pTIFR1Ptr;
#define TIFR1 *g_pTIFR1Ptr

///////////////////////////////////////
////////////////////////////////////////

class TIMSK1TestInterface
{
public:
	virtual ~TIMSK1TestInterface() = default;

	virtual TIMSK1TestInterface& operator |= (uint8_t) = 0;
	virtual TIMSK1TestInterface& operator &= (uint8_t) = 0;
};

extern TIMSK1TestInterface *g_pTIMSK1Ptr;
#define TIMSK1 *g_pTIMSK1Ptr


#endif //IO_H
