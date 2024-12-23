#ifndef MOCKS_H
#define MOCKS_H

#include <avr/io.h>
#include <gmock/gmock.h>

class MockTCNT1 final : public TCNT1TestInterface {
public:
	MOCK_CONST_METHOD1(AssignOp, void(uint16_t val));
	MockTCNT1& operator = (const uint16_t val) override { AssignOp(val); return *this; }

	MOCK_CONST_METHOD0(GetOp, uint16_t());
	operator uint16_t() const override { return GetOp(); }
};

class MockTIFR1 final : public TIFR1TestInterface {
public:
	MOCK_CONST_METHOD1(OrOp, void(uint8_t val));
	MockTIFR1& operator |= (const uint8_t val) override { OrOp(val); return *this; }
};

class MockTIMSK1 final : public TIMSK1TestInterface {
public:
	MOCK_CONST_METHOD1(OrOp, void(uint8_t val));
	MockTIMSK1& operator |= (const uint8_t val) override { OrOp(val); return *this; }

	MOCK_CONST_METHOD1(AndOp, void(uint8_t val));
	MockTIMSK1& operator &= (const uint8_t val) override { AndOp(val); return *this; }
};

#endif //MOCKS_H
