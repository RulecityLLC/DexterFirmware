#include <avr/io.h>
#include "common.h"
#include "led_driver.h"
#include "buttons.h"
#include "protocol.h"
#include "settings.h"
#include "timer-global.h"
#include "rev.h"

static uint8_t g_bModePressed = 1;	// initialize to being pressed to avoid power-up/reset jitter
static uint8_t g_bModePressedEventLatched = 0;	// 1 = mode pressed event should fire once button is released
static uint8_t g_bModePressedEventFired = 0;	// 1 = we've determined that the mode button was pressed
static uint8_t g_bModeHeldEventFired = 0;	// 1 = we've determined that the mode button was held and have called the appropriate event
static uint8_t g_bButtonPullupHasStablized = 0;	// so we don't get a false button press on reset

static uint8_t g_bDiagPressed = 1;	// initialize to being pressed to avoid power-up/reset jitter
static uint8_t g_bDiagEventFired = 0;
static uint8_t g_bReadyForDiagPress = 0;

// global timer value for debouncing button presses
static uint8_t g_u8ButtonTimerStart = 0;

// so we can detect if mode button has been held down
static uint16_t g_u16ModeButtonSlowTimerStart = 0;

// number counter must be to count as legit and not just random noise
#define PRESSED_THRESHOLD 200

// * 2 for 2 seconds, slow timer is cpu frequency divided by 1024 (timer divider) and then 256 (8-bit range)
#define HELD_THRESHOLD ((MY_F_CPU / 1024 / 256) * 2)

void buttons_init()
{
	SETUP_BUTTONS();
}

void buttons_think()
{
	uint8_t u8TimerDiffSinceStart = GLOBAL_TIMER_DIFF_SINCE_START(g_u8ButtonTimerStart);
	uint16_t u16SlowTimerDiffSinceStart = GLOBAL_SLOW_TIMER_DIFF_SINCE_START(g_u16ModeButtonSlowTimerStart);

	// MODE BUTTON 

	// if MODE button is being pressed (active low)
	if (BUTTON_MODE == 0)
	{
		// if we haven't just recently reset the system
		if (g_bButtonPullupHasStablized)
		{
			// if button was not previously pressed
			if (!g_bModePressed)
			{
				g_u8ButtonTimerStart = GET_GLOBAL_TIMER_VALUE();	// so we can detect how long button has been pressed
				g_u16ModeButtonSlowTimerStart = GET_GLOBAL_SLOW_TIMER_VALUE();	// so we can detect how long button has been pressed
				g_bModePressed = 1;	// so we don't keep re-starting the timers over and over again
				g_bModePressedEventLatched = 0;
				g_bModePressedEventFired = 0;	// so events can happen once the timer condition has been satisfied
				g_bModeHeldEventFired = 0;
			}
			// if the MODE button had been held long enough to be acknowledged, and it can fire, then indicate that it should be fired when button is released.
			// I intentionally want this event to fire when MODE button is released, because
			//   I don't want the laserdisc player type to advance when the user is trying
			//	 to enable auto-detect mode.
			else if ((!g_bModePressedEventFired) && (u8TimerDiffSinceStart > PRESSED_THRESHOLD))
			{
				g_bModePressedEventLatched = 1;	// make sure this event gets fired when button is released unless the button is held longer
				g_bModePressedEventFired = 1;	// make sure the latch only gets set once
			}
			// else if we haven't done a mode held event yet and button has been pressed long enough...
			else if ((!g_bModeHeldEventFired) && (u16SlowTimerDiffSinceStart > HELD_THRESHOLD))
			{
				// do mode held event
				OnModeHeld();
				g_bModeHeldEventFired = 1;	// make sure this doesn't happen more than once
				g_bModePressedEventLatched = 0;	// make sure mode press event doesn't fire when button is released
			}
			// else they haven't held button down long enough to do anything yet
		}
	}
	// else MODE button is released
	else
	{
		// if button recently became released (could be voltage 'bounce' or someone really releasing the button)
		if (g_bModePressed)
		{
			// if we are supposed to fire the event when button is released
			if (g_bModePressedEventLatched)
			{
				// do mode pressed event
				OnModePressed();
				g_bModePressedEventLatched = 0;	// make sure this doesn't get fired more than once
			}

			g_u8ButtonTimerStart = GET_GLOBAL_TIMER_VALUE();	// so we can detect when the pull-up resistor has stablized
			g_bModePressed = 0;
		}
		// else if button has been released for a while, we can assume that the system has not recently been reset
		else if (u8TimerDiffSinceStart > PRESSED_THRESHOLD)
		{
			g_bButtonPullupHasStablized = 1;
		}
	}

	// DIAGNOSE BUTTON

	// if DIAGNOSE button is being pressed (active low)
	if (BUTTON_DIAGNOSE == 0)
	{
		// if button has been released long enough
		if (g_bReadyForDiagPress)
		{
			// if button was not previously pressed
			if (!g_bDiagPressed)
			{
				g_u8ButtonTimerStart = GET_GLOBAL_TIMER_VALUE();	// reset counter
				g_bDiagPressed++;
				g_bDiagEventFired = 0;
			}
			// else if we haven't done an event yet and button has been pressed long enough...
			else if ((!g_bDiagEventFired) && (u8TimerDiffSinceStart > PRESSED_THRESHOLD))
			{
				// do event
				OnDiagnosePressed();

				g_bDiagEventFired++;
				g_bReadyForDiagPress = 0;	// no presses until they release the button
			}
			// else event was already fired so ignore timer
		}
		// else it's too soon for another press
	}
	// else DIAGNOSE button is released
	else
	{
		// if it was just barely released, it needs to stay released before we acknowledge it
		if (g_bDiagPressed)
		{
			g_u8ButtonTimerStart = GET_GLOBAL_TIMER_VALUE();	// reset counter
			g_bDiagPressed = 0;
			g_bDiagEventFired = 0;
		}
		// else if it's been released long enough
		else if ((!g_bDiagEventFired) && (u8TimerDiffSinceStart > PRESSED_THRESHOLD))
		{
			g_bDiagEventFired++;
			g_bReadyForDiagPress++;	// button has officially been released
		}
	}

}

