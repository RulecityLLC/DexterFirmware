#include "mockLD700Callbacks.h"

static ILD700CallbacksTestInterface *g_pInstance;

// each test needs to call this
void mockLD700CallbacksSetInstance(ILD700CallbacksTestInterface *pInstance)
{
	g_pInstance = pInstance;
}

LD700Status_t ld700_convert_status(LDPCStatus_t src)
{
	return g_pInstance->ConvertStatus(src);
}

void ld700_close_tray()
{
	g_pInstance->CloseTray();
}

void ld700_eject()
{
	g_pInstance->Eject();
}
