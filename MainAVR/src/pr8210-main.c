#include <avr/io.h> 
#include <avr/interrupt.h>
#include <string.h>
#include "vldp-avr.h"
#include <ldp-abst/ldpc.h>
#include <ldp-abst/ldp_search_delay.h>
#include "protocol.h"
#include "settings.h"
#include "strings.h"
#include "idle.h"
#include "vsync.h"
#include "util.h"
#include "led_driver.h"
#include "pr8210-main.h"
#include "pr8210-common.h"
#include "pr8210-callbacks.h"
#include "vbi_inject.h"
#include "timer1.h"
#include "common-ldp.h"
#include "common.h"

/////////////////////////////////////////////////////////////////

// shared by vsync ISR and main pr8210 loop so needs to be volatile
volatile uint8_t g_u8PR8210VsyncActive = 0;	// whether vsync is active or not

// shared by vsync ISR and main pr8210 loop so needs to be volatile
volatile uint8_t g_u8PR8210AIntExtRaised = 1;	// default to raised (internal active)

// used only by ISR so does not need to be volatile
uint8_t g_u8PR8210AJmpTrigRaised = 1;	// default to raised (disabled)

// to track whether we receive a jump trigger before line 10 or so
// (shared with ISR, so needs to be volatile)
volatile uint8_t g_u8PR8210AGotJumpTriggerThisField = 0;

// whether we currently are receiving a message
volatile uint8_t g_pr8210_u8ReceivingMessage = 0;

// current message we're receiving
volatile uint16_t g_pr8210_u16Message = 0;

// how many bits we've received in current message
volatile uint8_t g_pr8210_u8BitCount = 0;

// finished message we've received and need processed
volatile uint16_t g_pr8210_u16FinishedMessage = 0;

// non-zero means we have a finished message to process
volatile uint8_t g_pr8210_u8FinishedMessageReady = 0;

// used for diagnostics
volatile uint8_t g_u8PR8210AVbiDataRaised = 0;

void pr8210_main_loop(LDPType ldptype)
{
	uint8_t g_u8PR8210CurField = 0;

	uint32_t u32VbiLines[3] = { 0, 0, 0};
	LDPCStatus_t uPR8210LdpStatus = 0;

	// Setup 16-bit timer
	// (other drivers may change this so we must set it here)

	// setup 16-bit timer, with CTC mode to detect timeouts
	TCCR1A = 0;
	TCCR1B = (1 << CS10) | (1 << WGM12);	// no prescaling (search datasheet for TCCR1B for details), CTC mode enabled

	// setup 8-bit timer, no CTC mode, /256 prescaling (so it has the same range as 16-bit timer with no pre-scaling)
	TCCR2A = 0;
	TCCR2B = (1 << CS22) | (1 << CS21);

	// disable serial port so we can use VBI injector
	DISABLE_DCE_AND_DTE();

	// opto relay must be disabled for VBI injection to work (by design, because VP931 shares TX1/RX1 lines)
	DISABLE_OPTO_RELAY();

	pr8210_setup_callbacks();

	// setup input pins
	SETUP_PR8210_INPUT();

	// setup output pins (for PR-8210A)
	SETUP_PR8210_OUTPUT();

	// prepare to receive interrupts
	SETUP_PR8210_INT();

	// enable vbi injection
	vbi_inject_init();

	// don't wanna hold onto this stack memory
	{
		char s1[20];
		char s[30];
		string_to_buf(s, ldptype == LDP_PR8210 ? STRING_PR8210 : STRING_PR8210A);
		string_to_buf(s1, STRING_START);
		strcat(s, s1);
		LOG(s);
	}

	// PR-8210 and PR-8210A modes have some hardware conflicts, so PR-8210A must be disabled if PR-8210 mode is active
	if (ldptype == LDP_PR8210)
	{
		DISABLE_PR8210A_MODE();
	}
	else
	{
		ENABLE_PR8210A_MODE();
	}

	pr8210i_reset();

	set_vsync_isr_callback(pr8210_vsync_callback);
	ENABLE_VSYNC_INT();

	set_timer1_isr_callback(pr8210_timer1_callback);

	// force seek delay to be longer so that (among other things), Us vs Them can correctly boot
	common_ldp_set_minimum_search_delay_ms(250);

	// start receiving interrupts from PR8210 pin
	ENABLE_PR8210_INT();

	// Go until our settings change
	while (!g_bRestartPlayer)
	{
		// wait for vsync to start
		while (!g_u8PR8210VsyncActive)
		{
			pr8210_think();

			// use idle time to process low-priority tasks
			idle_think();

			vbi_inject_think();

 			// if user presses MODE button then abort (this is needed in case we have no video signal plugged in)
			if (g_bRestartPlayer)
			{
				goto done;
			}
		}

		// we are entering the AUX AVR critical section when nothing should be sent to it (if data does get sent, it will get lost)
		vbi_inject_tx_enable(0);

		// TCNT2 has been reset by vsync ISR

		// Use the real field value (instead of simulated) so that we are in sync with VBI injection
		// The LM1881 uses 0 for even/bottom and 1 for top/odd (backward from our convention) so we need to flip it.
		// NOTE : VBI injection will be behind a whole frame if we do this, but it can't be helped.
		// We used to use the next field instead of the current field, but this caused a 'wobble' effect when Star Rider performed searches.
		g_u8PR8210CurField = FIELD_PIN ^ 1;

		// Wait until line 11 has passed to see if star rider is going to send us any jump triggers
		// (if it is going to send them, it sends one in the middle of line 9)
		// Vsync starts on line 4, so 11-4 is 7.
		// (63.555 uS per line, 7 lines is 445 uS)
		// Cycles til that time is 18.432 (cpu mhz) * 445 (microseconds)
		// Resulting cycle count is 8200, divide by 256 (prescaler enabled) to adjust to 8-bit timer2
		while (TCNT2 < 32)
		{
			// if a command does come in during this waiting period, it will mean that the vbi data will get sent slightly later which should be okay
			pr8210_think();
		}

		// if we've received a track jump this field, then disable auto track jump because it would just interfere
		// (star rider is trying to control the track jumps)
		if (g_u8PR8210AGotJumpTriggerThisField != 0)
		{
			ldpc_set_disable_auto_track_jump(LDPC_TRUE);

			// for logic analyzer diagnostics, not used by Star Rider
			LOWER_PR8210A_VBI_DATA();
		}
		// else if jump trigger int/ext' is in internal mode, enable auto track jump
		else if (g_u8PR8210AIntExtRaised)
		{
			ldpc_set_disable_auto_track_jump(LDPC_FALSE);

			// for logic analyzer diagnostics, not used by Star Rider
			RAISE_PR8210A_VBI_DATA();
		}
		// else leave it the way it was before

		ldpc_OnVBlankChanged(
			LDPC_TRUE,	// vblank _is_ active
			g_u8PR8210CurField);

		pr8210i_on_vblank();	// handle STAND BY blinker among other things

		// We don't really care when vsync ends, so we just pretend that it has ended immediately
		g_u8PR8210VsyncActive = 0;

		ldpc_OnVBlankChanged(
			LDPC_FALSE,	// vblank is no longer active (or at least, this is a good place to end it)
			g_u8PR8210CurField);

		// intentionally doing this before the TCNT2 delay to be efficient
		// (my coding style would prefer to put this right before it gets used)
		uPR8210LdpStatus = ldpc_get_status();

		// do generic video work
		on_video_field();

		// don't send any VBI inject updates until line 18 has passed because the AUX AVR will be in the middle of its critical section.
		// (63.555 uS per line, line 18 is 1144 uS, line 19 is 1207 uS)
		// Cycles til that time is 18.432 (cpu mhz) * 1207 (microseconds)
		// Resulting cycle count is 22247, divide by 256 (prescaler enabled) to adjust to 8-bit timer2
		while (TCNT2 < 87)
		{
			// if a command does come in during this waiting period, it will mean that the vbi data will get sent slightly later which should be okay
			pr8210_think();
		}

		// we are beyond the critical section, it's now safe to send stuff to the AUX AVR
		vbi_inject_tx_enable(1);

		// we don't want to send any vbi data while disc is stopped because it is too hard for AVR to recover if something goes wrong
		// (stopped VBI data has a lot of 00's in it which makes it hard to find the beginning of a valid packet again)
		if (uPR8210LdpStatus != LDPC_STOPPED)
		{
			// if we're playing or paused, then send VBI data
			if ((uPR8210LdpStatus == LDPC_PLAYING) || (uPR8210LdpStatus == LDPC_PAUSED))
			{
				// whether to send white flag
				uint8_t bWhiteFlag = 0;

				// set squelch high (inactive)
	        	DISABLE_PR8210A_SQUELCH(); 

				// line 17 and 18 are the same, line 16 will always be blank
				// NOTE : this VBI will actually be for the _next_ field, not the current field (ie we are queueing up the VBI injector)
				u32VbiLines[1] = u32VbiLines[2] = ldpc_get_current_field_vbi_line18();

				// MACH 3 and white flag:
				// The PR-8210 suppresses all white flags and MACH 3 expects to never receive them.
				// However, in our tests, non picture number fields have been getting parsed as garbage
				//  by the MACH 3 hardware.
				// Warren discovered that if a solid white line is sent for non-picture number fields,
				//  it resolves this problem.  Therefore, we send a white flag for all non-picture number fields.
				if (((u32VbiLines[1] >> 16) & 0xF8) != 0xF8)
				{
					bWhiteFlag = 1;
				}

				// void VbiInjectSendVbiUpdate(uint8_t u8FieldFlag, uint32_t pu32Lines[]);
				VbiInjectSendVbiUpdate(
					(g_u8PR8210CurField << 1) |	// which field we're on
					bWhiteFlag,	// white flag
					u32VbiLines
					);
			}
			// we are searching or spinning up
			else if ((uPR8210LdpStatus == LDPC_SEARCHING) || (uPR8210LdpStatus == LDPC_SPINNING_UP))
			{
				// set squelch low (active)
	            ENABLE_PR8210A_SQUELCH();

				// PR-8210 mutes video signal during seeks, so we need to do the same (some hardware like MACH 3 requires this)
				VbiInjectSendVideoMute(g_bPR8210AStandByRaised);
			}
			// else unhandled case, so err on side of caution and don't send anything
		}

		// re-enable vsync interrupt and wait for the next one
		ENABLE_VSYNC_INT();
	}

done:

	// clean-up

	// revert to default
	common_ldp_set_minimum_search_delay_ms(0);

	// reset timer registers to defaults
	TCCR1B = 0;
	TCCR2B = 0;

	// make sure that this isn't active when PR-8210A isn't active because it will interfere with DB25 port among other things
	DISABLE_PR8210A_MODE();

	// stop receiving interrupts from PR8210 pin
	DISABLE_PR8210_INT();

	DISABLE_CTC_INT();

	DISABLE_VSYNC_INT();

	vbi_inject_shutdown();

}

void pr8210_think()
{
	// if we have a message ready to process
	if (g_pr8210_u8FinishedMessageReady)
	{
		// if diagnostics mode is enabled, add some verbose logging
		if (IsDiagnosticsEnabledEeprom())
		{
			MediaServerSendRxLog(g_pr8210_u16FinishedMessage >> 3);
		}

		pr8210i_write(g_pr8210_u16FinishedMessage);
		g_pr8210_u8FinishedMessageReady = 0;
	}
}

ISR(PCINT2_vect) 
{
	// store cycle value as close to when interrupt started to maximize accuracy
	uint16_t u16Timer = TCNT1;
	uint8_t u8JmpTrigIntExt = PR8210_JMPTRIG_INTEXT;

	// detect change in INT/EXT' pin
	if (g_u8PR8210AIntExtRaised != u8JmpTrigIntExt)
	{
		g_u8PR8210AIntExtRaised = u8JmpTrigIntExt;
		pr8210i_on_jmptrig_and_scanc_intext_changed(g_u8PR8210AIntExtRaised);
	}

	// if remote control is high, it's not interesting to us, we only care if the pin went low
	// (if we want to be proper, we should check to see whether remote control was low before, but in practice it never changes at the same time as jtrig int/ext')
	if (PR8210_REMOTE_CTRL != 0)
	{
		goto done;
	}

	// if we are not currently receiving a message
	if (g_pr8210_u8ReceivingMessage == 0)
	{
		g_pr8210_u8ReceivingMessage = 1;

		// clear message so we can start fresh
		g_pr8210_u16Message = 0;
	
		// reset bit count so we know we've received 0 bits
		g_pr8210_u8BitCount = 0;

		OCR1A = CYCLES_TIL_TIMEOUT;	// make interrupt occur when it's been too long since we've got another pulse

		// we are at beginning of new pulse, so reset timer to look for the next pulse
		TCNT1 = 0;	// make sure timer is cleared after OCR1A set but before we clear CTC flag, to ensure we don't have any false positives

		TIFR1 = (1 << OCF1A); // clear the CTC flag so we don't immediately trigger ISR (writing a logic one to the set flag clears it)
		ENABLE_CTC_INT();	// start waiting for timeout
		goto done;
	}
	// else if we are still receiving the pulse (which may be alternating highs/lows from MACH 3 hardware) then ignore everything
	else if (u16Timer < CYCLES_UNTIL_PULSE_END)
	{
		goto done;
	}
	// else if the interval is small enough, it's a 0 bit
	else if (u16Timer < CYCLES_TIL_ITS_A_1)
	{
		// 0 bit!
		g_pr8210_u16Message >>= 1;
	}
	// else interval is big enough that it's a 1 bit
	else
	{
		// 1 bit!
		g_pr8210_u16Message >>= 1;
		g_pr8210_u16Message |= (1 << 9);	// insert at the high bit (this makes it easier to parse)
	}

	// if program flow comes here, it means that we processed a 0 or 1 bit
	TCNT1 = 0;	// we are at beginning of new pulse, so reset timer to look for the next pulse
	g_pr8210_u8BitCount++;

	// if we've processed all 10 bits, we're done
	if (g_pr8210_u8BitCount >= 10)
	{
		g_pr8210_u16FinishedMessage = g_pr8210_u16Message;	// copy so we can start receiving the next one before this one is processed (probably not necessary, just a precaution)
		g_pr8210_u8FinishedMessageReady = 1;	// tell non-ISR code that finished message is ready

		// NOTE : we can't set g_pr8210_u8ReceivingMessage to 0 here because we are at the beginning of a pulse
		//  and still need to ignore the "jitter" sent from MACH3.
		// So we intentionally allow our timeout ISR to trigger.  MACH3 waits _much_ longer than this before sending the next command so it should be safe.
	}

done:
	return;
}

ISR(PCINT3_vect) 
{
	uint8_t u8 = PR8210_JMPTRIG;
	uint8_t u8ScanC = PR8210_SCANC;

    // detect change in JMP TRIG pin
	// TODO : this may be redundant since nothing else can generate an interrupt but a JMP TRIG change
	if (g_u8PR8210AJmpTrigRaised != u8)
	{
		g_u8PR8210AJmpTrigRaised = u8;
		pr8210i_on_jmp_trigger_changed(g_u8PR8210AJmpTrigRaised, u8ScanC);
    }
}
