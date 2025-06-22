#ifndef MOCKAUTODETECTDEPS_H
#define MOCKAUTODETECTDEPS_H

#include <gmock/gmock.h>
#include "autodetect-deps.h"

class IAutodetectDepsTestInterface
{
public:
	virtual uint8_t IsPC0Raised() = 0;
};

void mockAutodetectDepsSetInstance(IAutodetectDepsTestInterface *pInstance);

class MockAutodetectDepsTestInterface : public IAutodetectDepsTestInterface {
public:
	MOCK_METHOD0(IsPC0Raised, uint8_t());
};

#endif //MOCKAUTODETECTDEPS_H
