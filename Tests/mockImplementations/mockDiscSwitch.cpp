#include "mockDiscSwitch.h"

static IDiscSwitchTestInterface *g_pInstance;

// each test needs to call this
void mockDiscSwitchSetInstance(IDiscSwitchTestInterface *pInstance)
{
	g_pInstance = pInstance;
}

void disc_switch_initiate(uint8_t idDisc)
{
	g_pInstance->Initiate(idDisc);
}

DiscSwitchStatus_t disc_switch_get_status()
{
	return g_pInstance->GetStatus();
}

void disc_switch_end()
{
	return g_pInstance->End();
}
