#ifndef MOCKSETTINGS_H
#define MOCKSETTINGS_H

#include <gmock/gmock.h>
#include "settings.h"

class ISettingsTestInterface
{
public:
	virtual uint8_t GetActiveDiscIdMemory() = 0;
};

void mockSettingsSetInstance(ISettingsTestInterface *pInstance);

class MockSettingsTestInterface : public ISettingsTestInterface
{
public:
	MOCK_METHOD0(GetActiveDiscIdMemory, uint8_t());
};

#endif //MOCKSETTINGS_H
