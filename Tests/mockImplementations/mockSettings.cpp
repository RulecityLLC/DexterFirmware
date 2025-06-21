#include "mockSettings.h"

static ISettingsTestInterface *g_pInstance;

// each test needs to call this
void mockSettingsSetInstance(ISettingsTestInterface *pInstance)
{
	g_pInstance = pInstance;
}

void SetAutodetectedLDPType(LDPType type)
{
	g_pInstance->SetAutodetectedLDPType(type);
}

uint8_t GetActiveDiscIdMemory()
{
	return g_pInstance->GetActiveDiscIdMemory();
}
