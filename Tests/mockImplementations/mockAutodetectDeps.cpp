#include "mockAutodetectDeps.h"

static IAutodetectDepsTestInterface *g_pInstance;

// each test needs to call this
void mockAutodetectDepsSetInstance(IAutodetectDepsTestInterface *pInstance)
{
	g_pInstance = pInstance;
}

uint8_t IsPC0Raised()
{
	return g_pInstance->IsPC0Raised();
}
