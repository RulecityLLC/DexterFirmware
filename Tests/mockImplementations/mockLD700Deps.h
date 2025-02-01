#ifndef MOCKLD700DEPS_H
#define MOCKLD700DEPS_H

#include <ldp-in/ld700-interpreter.h>
#include <ldp-abst/ldpc.h>
#include <gmock/gmock.h>
#include <ld700-callbacks.h>

class ILD700DepsTestInterface
{
public:
	virtual uint8_t GetLD700CandidateSide() = 0;
	virtual void OnFlipDiscPressed(LD700Status_t status) = 0;
	virtual void OnFlipDiscHeld(LD700Status_t status) = 0;
	virtual uint8_t GetDiscSideByDiscId(uint8_t u8DiscId) = 0;
};

void mockLD700DepsSetInstance(ILD700DepsTestInterface *pInstance);

class MockLD700DepsTestInterface : public ILD700DepsTestInterface
{
public:
	MOCK_METHOD0(GetLD700CandidateSide, uint8_t());

	MOCK_METHOD1(OnFlipDiscPressed, void(LD700Status_t status));

	MOCK_METHOD1(OnFlipDiscHeld, void(LD700Status_t status));

	MOCK_METHOD1(GetDiscSideByDiscId, uint8_t(uint8_t));
};

#endif //MOCKLD700DEPS_H
