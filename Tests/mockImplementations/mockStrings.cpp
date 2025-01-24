#include "mockStrings.h"

static IStringsTestInterface *g_pInstance;

// each test needs to call this
void mockStringsSetInstance(IStringsTestInterface *pInstance)
{
	g_pInstance = pInstance;
}

void log_string(StringID id)
{
	return g_pInstance->LogString(id);
}
