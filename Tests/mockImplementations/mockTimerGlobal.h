#ifndef MOCKTIMERGLOBAL_H
#define MOCKTIMERGLOBAL_H

#include <gmock/gmock.h>
#include "timer-global.h"

class ITimerGlobalTestInterface
{
public:
	virtual uint16_t GetSlowTimerVal() = 0;
};

void mockTimerGlobalSetInstance(ITimerGlobalTestInterface *pInstance);

class MockTimerGlobalTestInterface : public ITimerGlobalTestInterface
{
public:
	MOCK_METHOD0(GetSlowTimerVal, uint16_t());
};

#endif //MOCKTIMERGLOBAL_H
