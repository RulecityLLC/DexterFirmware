#include "mockStrings.h"

static IStringsTestInterface *g_pInstance;

// each test needs to call this
void mockStringsSetInstance(IStringsTestInterface *pInstance)
{
	g_pInstance = pInstance;
}

void string_to_buf(char *s, StringID id)
{
	g_pInstance->StringToBuf(s, id);
}

void log_string(StringID id)
{
	return g_pInstance->LogString(id);
}
