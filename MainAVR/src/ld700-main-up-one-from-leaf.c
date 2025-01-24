#include <avr/io.h> 
#include "common.h"	// for MY_F_CPU
#include "timer-global.h"	// for button debouncing
#include "ld700-main-deps.h"	// for OnFlipDiscHeld, etc.
#include "common-ldp.h"	// for vsync counter

/////////////////////////////////////////////////////////////////

static uint8_t g_bFlipDiscPressed;	// initialize to being pressed to avoid power-up/reset jitter
static uint8_t g_bFlipDiscPressedEventLatched;	// 1 = button pressed event should fire once button is released
static uint8_t g_bFlipDiscPressedEventDetected;	// 1 = we've determined that the button was pressed
static uint8_t g_bFlipDiscHeldEventFired;	// 1 = we've determined that the button was held and have called the appropriate event

// global timer value for debouncing button presses
static uint8_t g_u8FlipDiscTimerStart;

// so we can detect if button has been held down
static uint16_t g_u16FlipDiscSlowTimerStart;

// number counter must be to count as legit and not just random noise
#define PRESSED_THRESHOLD 200

// * 2 for 2 seconds, slow timer is cpu frequency divided by 1024 (timer divider) and then 256 (8-bit range)
#define HELD_THRESHOLD ((MY_F_CPU / 1024 / 256) * 2)

static uint8_t g_bLD700OneSideLEDIsOn;

////////////////////////////////////////////

void ld700_up_one_from_leaf_reset()
{
	g_bFlipDiscPressed = 0;
	g_bFlipDiscPressedEventLatched = 0;
	g_bFlipDiscPressedEventDetected = 0;
	g_bFlipDiscHeldEventFired = 0;
	g_u8FlipDiscTimerStart = 0;
	g_u16FlipDiscSlowTimerStart = 0;
	g_bLD700OneSideLEDIsOn = 0;
}

void ld700_button_think()
{
	uint8_t u8TimerVal = GET_GLOBAL_TIMER_VALUE();	// this may change each time its read so we only want to read it once to be consistent
	uint8_t u8TimerDiffSinceStart = (uint8_t) (u8TimerVal - g_u8FlipDiscTimerStart);
	uint16_t u16SlowTimerDiffSinceStart = GLOBAL_SLOW_TIMER_DIFF_SINCE_START(g_u16FlipDiscSlowTimerStart);

	// if 'flip disc' button is being pressed
	if ((PINA & (1 << PA2)) == 0)
	{
		// if button was not previously pressed
		if (!g_bFlipDiscPressed)
		{
			g_u8FlipDiscTimerStart = u8TimerVal;	// restart debouncer
			g_u16FlipDiscSlowTimerStart = GET_GLOBAL_SLOW_TIMER_VALUE();	// restart debouncer
			g_bFlipDiscPressed++;	// so we don't keep re-starting the timers over and over again
				
			// the debouncer has started over, so no event is active
			g_bFlipDiscPressedEventLatched = 0;
			g_bFlipDiscPressedEventDetected = 0;
			g_bFlipDiscHeldEventFired = 0;
		}
		// detect the button being pressed.  We only want this detection to occur once.
		else if ((!g_bFlipDiscPressedEventDetected) && (u8TimerDiffSinceStart > PRESSED_THRESHOLD))
		{
			g_bFlipDiscPressedEventLatched++;	// make sure this event gets fired when button is released unless the button is held longer
			g_bFlipDiscPressedEventDetected++;	// make sure the latch only gets set once
		}
		// We only want to take action once if the button has been held.
		else if ((!g_bFlipDiscHeldEventFired) && (u16SlowTimerDiffSinceStart > HELD_THRESHOLD))
		{
			// do held event
			OnFlipDiscHeld();
			g_bFlipDiscHeldEventFired++;	// make sure this doesn't happen more than once
			g_bFlipDiscPressedEventLatched = 0;	// make sure press event doesn't fire when button is released because the user intended to hold the button, not press it quickly.
		}
	}
	// else MODE button is released
	else
	{
		// if button recently became released (could be voltage 'bounce' or someone really releasing the button)
		if (g_bFlipDiscPressed)
		{
			// if we are supposed to fire the event when button is released
			if (g_bFlipDiscPressedEventLatched)
			{
				// do pressed event
				OnFlipDiscPressed();
				g_bFlipDiscPressedEventLatched = 0;	// make sure this doesn't get called more than once
			}

			g_u8FlipDiscTimerStart = u8TimerVal;	// restart debouncer
			g_bFlipDiscPressed = 0;
		}
	}
}

// should only be called once per vsync
void ld700_on_vblank()
{
	if (common_get_3bit_vsync_counter() == 0)
	{
		// if we're blinking, we want to show the candidate that we will change to.  Otherwise we want to show the disc that's inserted.
		uint8_t u8Side = GetLD700CandidateSide();

		// blink the LED so we know the interrupt is working
		g_bLD700OneSideLEDIsOn ^= 1;	// toggle

       	// PA5: Side 2 LED
		// PA6: Side 1 LED (if SSR is enabled)

		// easy to turn both LEDs off if neither should be on
		if (!g_bLD700OneSideLEDIsOn)
		{
			PORTA &= ~((1 << PA5)|(1 << PA6));
		}
		else if (u8Side == 1)
        {
			PORTA &= ~(1 << PA5);
			PORTA |= (1 << PA6);
        }
        else if (u8Side == 2)
        {
			PORTA &= ~(1 << PA6);
			PORTA |= (1 << PA5);
        }
        // else disc is unknown so we'll illuminate both LEDs just so the user knows something is wrong
        else
		{
			PORTA |= ((1 << PA5)|(1 << PA6));
		}
	}
}
