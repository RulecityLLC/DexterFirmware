#include <ldp-abst/ldpc.h>
#include "mockLdpc.h"

static ILdpcTestInterface *g_pInstance;

// each test needs to call this
void mockLdpcSetInstance(ILdpcTestInterface *pInstance)
{
	g_pInstance = pInstance;
}

LDPCStatus_t ldpc_get_status()
{
	return g_pInstance->GetStatus();
}
