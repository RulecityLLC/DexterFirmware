#include "mockTimerGlobal.h"

static ITimerGlobalTestInterface *g_pInstance;

// each test needs to call this
void mockTimerGlobalSetInstance(ITimerGlobalTestInterface *pInstance)
{
	g_pInstance = pInstance;
}

uint16_t timer_global_get_slow_val()
{
	return g_pInstance->GetSlowTimerVal();
}
