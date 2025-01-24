#ifndef MOCKLD700CALLBACKS_H
#define MOCKLD700CALLBACKS_H

#include <ldp-in/ld700-interpreter.h>
#include <ldp-abst/ldpc.h>
#include <gmock/gmock.h>

class ILD700CallbacksTestInterface
{
public:
	virtual LD700Status_t ConvertStatus(LDPCStatus_t) = 0;
	virtual void CloseTray() = 0;
	virtual void Eject() = 0;
};

void mockLD700CallbacksSetInstance(ILD700CallbacksTestInterface *pInstance);

class MockLD700CallbacksTestInterface : public ILD700CallbacksTestInterface
{
public:
	MOCK_METHOD1(ConvertStatus, LD700Status_t(LDPCStatus_t));

	MOCK_METHOD0(CloseTray, void());

	MOCK_METHOD0(Eject, void());
};

#endif //MOCKLD700CALLBACKS_H
