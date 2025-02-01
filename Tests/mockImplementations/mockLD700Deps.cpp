#include "mockLD700Deps.h"

static ILD700DepsTestInterface *g_pInstance;

void mockLD700DepsSetInstance(ILD700DepsTestInterface *pInstance)
{
	g_pInstance = pInstance;
}

uint8_t GetLD700CandidateSide()
{
	return g_pInstance->GetLD700CandidateSide();
}

void OnFlipDiscPressed(LD700Status_t status)
{
	g_pInstance->OnFlipDiscPressed(status);
}

void OnFlipDiscHeld(LD700Status_t status)
{
	g_pInstance->OnFlipDiscHeld(status);
}

uint8_t GetDiscSideByDiscId(uint8_t u8DiscId)
{
	return g_pInstance->GetDiscSideByDiscId(u8DiscId);
}
