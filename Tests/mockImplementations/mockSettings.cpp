#include "mockSettings.h"

static ISettingsTestInterface *g_pInstance;

// each test needs to call this
void mockSettingsSetInstance(ISettingsTestInterface *pInstance)
{
	g_pInstance = pInstance;
}

uint8_t GetActiveDiscIdMemory()
{
	return g_pInstance->GetActiveDiscIdMemory();
}
