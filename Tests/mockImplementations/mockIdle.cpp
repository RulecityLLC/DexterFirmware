#include "mockIdle.h"

static IIdleTestInterface *g_pInstance;

// each test needs to call this
void mockIdleSetInstance(IIdleTestInterface *pInstance)
{
	g_pInstance = pInstance;
}

void idle_think()
{
	g_pInstance->IdleThink();
}
