#ifndef MOCKDISCSWITCH_H
#define MOCKDISCSWITCH_H

#include <gmock/gmock.h>
#include "disc_switch.h"

class IDiscSwitchTestInterface
{
public:
	virtual void Initiate(uint8_t discId) = 0;
	virtual DiscSwitchStatus_t GetStatus() = 0;
	virtual void Think() = 0;
	virtual void End() = 0;
};

void mockDiscSwitchSetInstance(IDiscSwitchTestInterface *pInstance);

class MockDiscSwitchTestInterface : public IDiscSwitchTestInterface
{
public:
	MOCK_METHOD1(Initiate, void(uint8_t));

	MOCK_METHOD0(GetStatus, DiscSwitchStatus_t());

	MOCK_METHOD0(Think, void());

	MOCK_METHOD0(End, void());
};

#endif //MOCKDISCSWITCH_H
