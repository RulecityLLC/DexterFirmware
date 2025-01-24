#ifndef MOCKS_H
#define MOCKS_H

#include <avr/io.h>
#include <gmock/gmock.h>

class MockReadOnlyRegister8 final : public ReadOnlyRegister<uint8_t>  {
public:
	MOCK_CONST_METHOD0(GetOp, uint8_t());
	operator uint8_t() const override { return GetOp(); }
};

class MockReadWriteRegister8 final : public ReadWriteRegister<uint8_t> {
public:
	MOCK_CONST_METHOD1(AssignOp, void(uint8_t val));
	MockReadWriteRegister8& operator = (const uint8_t val) override { AssignOp(val); return *this; }

	MOCK_CONST_METHOD1(AndEqualsOp, void(uint8_t val));
	MockReadWriteRegister8& operator &= (const uint8_t val) override { AndEqualsOp(val); return *this; }

	MOCK_CONST_METHOD1(OrEqualsOp, void(uint8_t val));
	MockReadWriteRegister8& operator |= (const uint8_t val) override { OrEqualsOp(val); return *this; }

	MOCK_CONST_METHOD0(GetOp, uint8_t());
	operator uint8_t() const override { return GetOp(); }
};

class MockReadWriteRegister16 final : public ReadWriteRegister<uint16_t> {
public:
	MOCK_CONST_METHOD1(AssignOp, void(uint16_t val));
	MockReadWriteRegister16& operator = (const uint16_t val) override { AssignOp(val); return *this; }

	MOCK_CONST_METHOD1(AndEqualsOp, void(uint16_t val));
	MockReadWriteRegister16& operator &= (const uint16_t val) override { AndEqualsOp(val); return *this; }

	MOCK_CONST_METHOD1(OrEqualsOp, void(uint16_t val));
	MockReadWriteRegister16& operator |= (const uint16_t val) override { OrEqualsOp(val); return *this; }

	MOCK_CONST_METHOD0(GetOp, uint16_t());
	operator uint16_t() const override { return GetOp(); }
};

#endif //MOCKS_H
