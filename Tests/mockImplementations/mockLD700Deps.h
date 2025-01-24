#ifndef MOCKLD700DEPS_H
#define MOCKLD700DEPS_H

#include <ldp-in/ld700-interpreter.h>
#include <ldp-abst/ldpc.h>
#include <gmock/gmock.h>

class ILD700DepsTestInterface
{
public:
	virtual uint8_t GetLD700CandidateSide() = 0;
	virtual void OnFlipDiscPressed() = 0;
	virtual void OnFlipDiscHeld() = 0;
};

void mockLD700DepsSetInstance(ILD700DepsTestInterface *pInstance);

class MockLD700DepsTestInterface : public ILD700DepsTestInterface
{
public:
	MOCK_METHOD0(GetLD700CandidateSide, uint8_t());

	MOCK_METHOD0(OnFlipDiscPressed, void());

	MOCK_METHOD0(OnFlipDiscHeld, void());
};

#endif //MOCKLD700DEPS_H
