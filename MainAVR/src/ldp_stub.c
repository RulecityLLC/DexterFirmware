#include "ldp_stub.h"
#include "strings.h"
#include "vldp-avr.h"
#include "idle.h"

void ldp_stub_main_loop()
{
	log_string(STRING_UNIMPLEMENTED);

	// wait to change to a new LDP type
	while (!g_bRestartPlayer)
	{
		idle_think();
	}
}
