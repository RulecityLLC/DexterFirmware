#include "mockCommonLdp.h"

static ICommonLdpTestInterface *g_pInstance;

// each test needs to call this
void mockCommonLdpSetInstance(ICommonLdpTestInterface *pInstance)
{
	g_pInstance = pInstance;
}

uint8_t common_get_3bit_vsync_counter()
{
	return g_pInstance->Get3bitVsyncCounter();
}
