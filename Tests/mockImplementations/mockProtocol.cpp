#include "mockProtocol.h"

static IProtocolTestInterface *g_pInstance;

// each test needs to call this
void mockProtocolSetInstance(IProtocolTestInterface *pInstance)
{
	g_pInstance = pInstance;
}

void MediaServerSendLog(const char *s)
{
	g_pInstance->MediaServerSendLog(s);
}
