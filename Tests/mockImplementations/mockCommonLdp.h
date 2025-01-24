#ifndef MOCKCOMMONLDP_H
#define MOCKCOMMONLDP_H

#include <gmock/gmock.h>
#include "disc_switch.h"

class ICommonLdpTestInterface
{
public:
	virtual uint8_t Get3bitVsyncCounter() = 0;
};

void mockCommonLdpSetInstance(ICommonLdpTestInterface *pInstance);

class MockCommonLdpTestInterface : public ICommonLdpTestInterface
{
public:
	MOCK_METHOD0(Get3bitVsyncCounter, uint8_t());
};

#endif //MOCKCOMMONLDP_H
