#ifndef MOCKIDLE_H
#define MOCKIDLE_H

#include <gmock/gmock.h>
#include "idle.h"

class IIdleTestInterface
{
public:
	virtual void IdleThink() = 0;
};

void mockIdleSetInstance(IIdleTestInterface *pInstance);

class MockIdleTestInterface : public IIdleTestInterface
{
public:
	MOCK_METHOD0(IdleThink, void());
};

#endif //MOCKIDLE_H
