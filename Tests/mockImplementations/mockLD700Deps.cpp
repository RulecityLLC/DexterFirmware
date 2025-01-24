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

void OnFlipDiscPressed()
{
	g_pInstance->OnFlipDiscPressed();
}

void OnFlipDiscHeld()
{
	g_pInstance->OnFlipDiscHeld();
}

