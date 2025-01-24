#include <avr/eeprom.h>
#include <avr/interrupt.h>	// to call cli()
#include <string.h>	// for memcpy
#include "settings.h"
#include "protocol.h"
#include "common.h"	// to disable interrupt
#include "vsync.h"	// to disable interrupt
#include "serial.h"	// for debug
#include "serial2.h"	// to disable interrupt
#include "dexter_strings.h"
#include "vldp-avr.h"	// for restart player global
#include "led_driver.h"
#include "common-ldp.h"
#include "deferred_eeprom_write.h"
#include "crc.h"
#include <ldp-abst/VBICompact.h>

// holds our settings in EEPROM
uint16_t EEMEM SettingsLength;
uint16_t EEMEM SettingsCRC;
uint8_t EEMEM Settings[500];	// these present the HELLO RESPONSE we got from the server, or blank if we didn't get one

//////////////////////////////////////////////

// holds our settings in RAM
uint8_t g_settings[sizeof(Settings)];

// size of settings in bytes
uint16_t g_uSettingsLength;

// points to VBI entries, used the LDPC
VBICompact_t g_compact;

LDPType g_LdpTypeAutodetected = LDP_NONE;	// initialize explicitly so we don't get bogus behavior
LDPType g_LdpTypeActive = LDP_NONE;	// initialize explicitly so we don't get bogus behavior

// holds the active disc id (which we do not want stored in eeprom)
uint8_t g_u8ActiveDiscId = 0;

// normally holds value from eeprom but can be overriden by Merlin
uint8_t g_bSearchDelayEnabled = 1;

// to make sure that the copy from eeprom only happens once
uint8_t g_bInitialSettingsCopiedFromEeprom = 0;

// whether ld-v1000 'super' (universal) mode is enabled.  Should usually by disabled.
uint8_t g_bSuperModeEnabled = 0;

// whether stop codes are honored (by default, they are)
uint8_t g_bHonorStopCodes = 1;

// LDP behavior.  A bit being cleared means authentic behavior, a bit being set means modified behavior.  See defines below.
#define NO_SPINUP_DELAY (1 << 0)
#define NO_SEEK_DELAY (1 << 1)
#define NO_SEEK_BLANKING (1 << 2)

// change this version any time the meaning of the cached eeprom settings changes
#define EEPROM_SETTINGS_VERSION 2

///////////////////////////////////////////////////////////////////////////

uint16_t load_settings_helper(void *pDstBuf)
{
	uint8_t *p8Buf = (uint8_t *) pDstBuf;
	uint16_t uSettingsLength = eeprom_read_word(&SettingsLength);
	uint16_t uSettingsCRC = eeprom_read_word(&SettingsCRC);

	uSettingsLength = eeprom_read_word(&SettingsLength);
	uSettingsCRC = eeprom_read_word(&SettingsCRC);

	// if the setting length is obviously wrong, then don't bother doing the CRC check
	if (uSettingsLength > sizeof(g_settings))
	{
		goto settings_bad;
	}

	eeprom_read_block(pDstBuf, &Settings, uSettingsLength);

	crc_ccitt_init();

	// compute CRC
	for (unsigned int i = 0; i < uSettingsLength; i++)
	{
		crc_ccitt_update(p8Buf[i]);
	}

	// add version to CRC so that we can invalidate old settings if their meaning changes
	crc_ccitt_update(EEPROM_SETTINGS_VERSION);

	// if CRC does not match, then fill in the buffer with default values
	if (uSettingsCRC == crc_ccitt_crc())
	{
		goto apply_settings;
	}

settings_bad:
	log_string(STRING_DEFAULT);

	// fill in default values since the CRC failed

	p8Buf[0] = p8Buf[1] = 0;	// persistent identifier
	p8Buf[2] = LDP_LDV1000;	// default player
	p8Buf[3] = 0 | (1 << 6);	// behavior is authentic with auto-detection enabled
#ifdef DEBUG
	p8Buf[3] |= (1 << 3);	// enable diagnose mode
#endif
	p8Buf[4] = 0;	// no auto detection strategy

	p8Buf[5] = 0;	// disc id not specified
	p8Buf[6] = p8Buf[7] = p8Buf[8] = p8Buf[9] = p8Buf[10] =
		p8Buf[11] = p8Buf[12] = p8Buf[13] = p8Buf[14] = 0;	// no discs available for switching
	p8Buf[15] = 0;	// mandatory null terminator

	p8Buf[16] = p8Buf[17] = 0;	// dynamic identifier

	// auto-generate default VBI pattern
	{
		VBICompactEntry_t entry;
		VBICompact_t compact;

		compact.pEntries = &entry;
		compact.uEntryCount = 1;
		compact.uTotalFields = 80000;	// arbitrary

		entry.u32StartAbsField = 0;
		entry.i32StartPictureNumber = 1;
		entry.typePattern = PATTERN_22;
		entry.u16Special = 0;
		entry.u8PatternOffset = 0;

		uSettingsLength = SETTINGS_BYTES_BEFORE_VBI_PATTERN + VBIC_ToBuffer(p8Buf + SETTINGS_BYTES_BEFORE_VBI_PATTERN, sizeof(Settings) - SETTINGS_BYTES_BEFORE_VBI_PATTERN, &compact);
	}

	// don't need to save settings to eeprom because we can always generate them again

apply_settings:

	g_uSettingsLength = uSettingsLength;

	// need to setup our LDPC functions with the settings in order for it to work correctly
	post_apply_settings();

	return uSettingsLength;
}

void load_settings_from_eeprom()
{
	load_settings_helper(g_settings);
}

void apply_new_settings(uint8_t u8OverwriteEeprom, const void *pSrcBuf, uint16_t stBufSize)
{
	// if we are to overwrite the EEPROM, then do so now
	if (u8OverwriteEeprom != 0)
	{
		if (!save_settings_to_eeprom(pSrcBuf, stBufSize))
		{
			log_string(STRING_SAVE_SETTINGS_FAILED);
			return;
		}
	}

	// if user presses a button on the PCB, then this function will be called with g_settings as the src buf, so
	//  we need to make sure we don't try to copy a buffer on to itself in this case
	if(pSrcBuf != g_settings)
	{
		// copy over the settings into our RAM so that calls like GetLDPType() will work
		memcpy(g_settings, pSrcBuf, stBufSize);
		g_uSettingsLength = stBufSize;
	}

	// settings have changed so we need to re-apply them
	post_apply_settings();
}

uint8_t save_current_settings_to_eeprom()
{
	return save_settings_to_eeprom(g_settings, g_uSettingsLength);
}

uint8_t save_settings_to_eeprom(const void *pSrcBuf, uint16_t stBufSize)
{
	const uint8_t *p8Buf = (const uint8_t *) pSrcBuf;
	uint16_t uSettingsCRC = 0;

	// safety check
	if (stBufSize > sizeof(Settings))
	{
		return 0;
	}

	crc_ccitt_init();

	// compute CRC
	for (uint16_t i = 0; i < stBufSize; i++)
	{
		crc_ccitt_update(p8Buf[i]);
	}

	// add version to CRC so that we can invalidate old settings if their meaning changes
	crc_ccitt_update(EEPROM_SETTINGS_VERSION);

	uSettingsCRC = crc_ccitt_crc();

	// save to EEPROM
	eeprom_write_word(&SettingsLength, stBufSize);
	eeprom_write_word(&SettingsCRC, uSettingsCRC);
	eeprom_write_block(pSrcBuf, &Settings, stBufSize);

	// to help us ensure that we are not writing to the EEPROM more than necessary
	log_string(STRING_EEPROM_WRITTEN);

	return 1;
}

void post_apply_settings()
{
	// set up VBICompact struct

	// the entries array will start after the N bytes indicating laserdisc behavior, and the 6 bytes indicating entry/field count
	g_compact.pEntries = (VBICompactEntry_t *) &g_settings[SETTINGS_BYTES_BEFORE_VBI_PATTERN + 6];
	g_compact.uEntryCount = g_settings[SETTINGS_BYTES_BEFORE_VBI_PATTERN + 1];	// this offset is the entry count by definition
	g_compact.uTotalFields = g_settings[SETTINGS_BYTES_BEFORE_VBI_PATTERN + 2] |
		((uint16_t) g_settings[SETTINGS_BYTES_BEFORE_VBI_PATTERN + 3] << 8) |
		((uint32_t) g_settings[SETTINGS_BYTES_BEFORE_VBI_PATTERN + 4] << 16) |
		((uint32_t) g_settings[SETTINGS_BYTES_BEFORE_VBI_PATTERN + 5] << 24);	// little-endian

	// now initialize the LDPC functions with our VBI
	ldpc_init(GetDiscTypeEeprom(), &g_compact);

	// set spin-up delay
	common_enable_spinup_delay(IsSpinupDelayEnabledEeprom());

	// disable super mode by default so we have a clean state to start in
	g_bSuperModeEnabled = 0;

	// we only want to copy from eeprom once.  if merlin auto-switches us to a different disc, we do not want to revert to the eeprom settings.
	if (!g_bInitialSettingsCopiedFromEeprom)
	{
		// set search memory search delay value to be equal to what is in the eeprom
		// (things like Merlin can change this at run-time)
		SetSearchDelayEnabledMemory(IsSearchDelayEnabledEeprom());

		// we only want to set the active disc id once (to the default disc id)
		SetActiveDiscIdMemory(GetDefaultDiscIdEeprom());

		g_bInitialSettingsCopiedFromEeprom = 1;
	}

	// log that new settings have been applied
	log_string(STRING_SETTINGS_APPLIED);
}

uint8_t IsSpinupDelayEnabledEeprom()
{
	// bit 0: clear if authentic spinup delay, set if fast/minimal.
	// Needs to be flipped so that non zero is enabled and zero is disabled.
	return (g_settings[3] & 1) ^ 1;
}

LDPCDiscType_t GetDiscTypeEeprom()
{
	LDPCDiscType_t res = LDPC_DISC_NTSC;
	if (g_settings[3] & 0x80)
	{
		res = LDPC_DISC_PAL;
	}
	return res;
}

LDPType GetManualLDPTypeEeprom()
{
	return g_settings[2];
}

LDPType GetAutodetectedLDPType()
{
	return g_LdpTypeAutodetected;
}

void SetAutodetectedLDPType(LDPType type)
{
	g_LdpTypeAutodetected = type;
}

LDPType GetActiveLDPType()
{
	return g_LdpTypeActive;
}

void SetActiveLDPType(LDPType type)
{
	g_LdpTypeActive = type;
}

uint16_t GetPersistentIdentifierEeprom()
{
	// little-endian formatted
	return (g_settings[0] | ((uint16_t) g_settings[1] << 8));
}

uint16_t GetDynamicIdentifierEeprom()
{
	// little-endian formatted
	return (g_settings[16] | ((uint16_t) g_settings[17] << 8));
}

uint8_t IsSearchDelayEnabledEeprom()
{
	// bit 1 has the answer to this question
	//  but it is high if search delay is disabled, so we have to flip the bit.
	return (g_settings[3] & (1 << 1)) ^ (1 << 1);
}

uint8_t IsSearchDelayEnabledMemory()
{
	return g_bSearchDelayEnabled;
}

void SetSearchDelayEnabledMemory(uint8_t bEnabled)
{
	g_bSearchDelayEnabled = bEnabled;
}

uint8_t IsSearchBlankingEnabledEeprom()
{
	// bit 2 has the answer to this question
	//  bit is high if search blanking is disabled, so we have to flip the bit
	return (g_settings[3] & (1 << 2)) ^ (1 << 2);
}

uint8_t IsDiagnosticsEnabledEeprom()
{
	// bit 3 has the answer to this question
	return (g_settings[3] & (1 << 3));
}

uint8_t IsVideoMuteDuringSearchEnabledEeprom()
{
	// bit 4 1 has the answer to this question
	return (g_settings[3] & (1 << 4));
}

uint8_t Is4800BaudEnabledEeprom()
{
	// bit 5 is 1 if we should be at 4800 baud instead of 9600
	return (g_settings[3] & (1 << 5));
}

uint8_t IsAutoDetectionEnabledEeprom()
{
	// bit 6 is 1 if we should auto-detect, or 0 if we should not
	return (g_settings[3] & (1 << 6));
}

AutoDetectStrategy GetAutoDetectStrategyEeprom()
{
	return g_settings[4];
}

uint8_t GetDefaultDiscIdEeprom()
{
	return g_settings[5];
}

const uint8_t *GetAvailableDiscIdsEeprom()
{
	// intentionally including the active disc because the active disc is one of the available discs
	return &g_settings[5];
}

uint8_t GetActiveDiscIdMemory()
{
	return g_u8ActiveDiscId;
}

void SetActiveDiscIdMemory(uint8_t id)
{
	g_u8ActiveDiscId = id;
}

uint8_t IsSuperModeEnabledMemory()
{
	return g_bSuperModeEnabled;
}

void OnSuperModeChanged(uint8_t bEnabled)
{
	g_bSuperModeEnabled = bEnabled;

	g_bRestartPlayer++;	// this will exit ld-v1000/pr-7820 modes and restart us in super mode
}

uint8_t GetHonorStopCodesMemory()
{
	return g_bHonorStopCodes;
}

void SetHonorStopCodesMemory(uint8_t bHonored)
{
	g_bHonorStopCodes = bHonored;
}

void OnModePressed()
{
	// Super mode should always be disabled unless it is explicitly enabled by the game.
	// This is because Dexter needs a clean state to fall back to if something goes wrong.
	g_bSuperModeEnabled = 0;

	// get the current ldp type so we know which LDP type to change to
	LDPType cur = GetManualLDPTypeEeprom();
	LDPType next;

	// if auto detection is enabled, then don't second guess the user.  Assume that they really want to change the player type
	if (IsAutoDetectionEnabledEeprom())
	{
		g_settings[3] &= ~(1 << 6);	// disable auto-detection

		// it will probably be less confusing if we make the current type be whatever has been auto-detected
		// (this should still be ok even of no player has been auto-detected because our switch statement will handle it)
		cur = GetAutodetectedLDPType();
	}

#ifdef REV2
	switch (cur)
	{
	default:	// LD-V1000
	case LDP_LDV1000:
		next = LDP_LDV1000_BL;
		break;
	case LDP_LDV1000_BL:
		next = LDP_PR7820;
		break;
	case LDP_PR7820:
		next = LDP_PR8210A;
		break;
	case LDP_PR8210A:
		next = LDP_VP931;
		break;
	case LDP_VP931:
		next = LDP_VP932;
		break;
	case LDP_VP932:
		next = LDP_VP380;
		break;
	case LDP_VP380:
		next = LDP_SIMUTREK;
		break;
	case LDP_SIMUTREK:
		next = LDP_LDV8000;
		break;
	case LDP_LDV8000:
		next = LDP_LDP1450;
		break;
	case LDP_LDP1450:
		next = LDP_LDP1000A;
		break;
	case LDP_LDP1000A:
		next = LDP_VIP9500SG;
		break;
	case LDP_VIP9500SG:
		next = LDP_PR8210;
		break;
	case LDP_PR8210:
		next = LDP_LDV1000;
		break;
	}
#else // rev3
	switch (cur)
	{
	default:
	case LDP_LDV1000:
		next = LDP_LDV1000_BL;
		break;
	case LDP_LDV1000_BL:
		next = LDP_PR7820;
		break;
	case LDP_PR7820:
		next = LDP_PR8210A;
		break;
	case LDP_PR8210A:
		next = LDP_VP931;
		break;
	case LDP_VP931:
		next = LDP_VP932;
		break;
	case LDP_VP932:
		next = LDP_SIMUTREK;
		break;
	case LDP_SIMUTREK:
		next = LDP_LDP1450;
		break;
	case LDP_LDP1450:
		next = LDP_VIP9500SG;
		break;
	case LDP_VIP9500SG:
		next = LDP_OTHER;
		break;
	case LDP_LDP1000A:	// these modes could be differentiated internally, but they all share the 'Other' LED on the PCB so will all advance to the same destination when mode is pressed
	case LDP_LD700:
	case LDP_OTHER:
		next = LDP_PR8210;
		break;
	case LDP_PR8210:
		next = LDP_LDV1000;
		break;
	}
#endif // rev

	// in case media server is down, make sure we change to the new LDP type locally
	g_settings[2] = next;
	g_bRestartPlayer++;	// this will update the laserdisc LEDs and switch us to a new laserdisc type

	// tell media server about about the mode change so it can optionally take action
	MediaServerSendSettings();

	// we save the settings here (as opposed to having media server tell us to save settings) in case the media server is not present
	deferred_eeprom_write_restart();
}

// holding mode button enables/disables auto detection
void OnModeHeld()
{
	// Super mode should always be disabled unless it is explicitly enabled by the game.
	// This is because Dexter needs a clean state to fall back to if something goes wrong.
	g_bSuperModeEnabled = 0;

	g_settings[3] |= (1 << 6);	// always enable auto-detection if mode is held to make things simple for the user
	g_bRestartPlayer++;	// force LDP type to be re-auto-detected

	// tell media server about about the mode change so it can optionally take action
	MediaServerSendSettings();

	// we save the settings here (as opposed to having media server tell us to save settings) in case the media server is not present
	deferred_eeprom_write_restart();
}

void OnDiagnosePressed()
{
	g_settings[3] ^= (1 << 3);	// toggle bit

	MediaServerSendSettings();	// tell media server that diagnostics mode has changed

	update_diagnose_led();

	// we save the settings here (as opposed to having media server tell us to save settings) in case the media server is not present
	deferred_eeprom_write_restart();
}

/////////////

void ForceReprogram()
{
	// tell bootloader that it needs to reprogram
	eeprom_update_word((uint16_t *) EEPROM_SIZE_BYTES - 2, 0);

	// disable all interrupts so bootloader runs properly
	EIMSK = 0;	// disable all external interrupts
	EICRA = 0;	// reset to default
	EIFR = EIFR;	// clear all interrupt flags
	PCMSK0 = PCMSK1 = PCMSK2 = PCMSK3 = 0;	// disable all pin change interrupts
	PCICR = PCICR;	// clear all interrupt flags
	TIMSK0 = TIMSK1 = TIMSK2 = 0;	// disable all timer interrupts
	serial_shutdown();	// disable serial port interrupt
	serial2_shutdown();
	cli();	// disable all interrupts

	// jump to the bootloader now
	//asm volatile("jmp 0x7C00"::);
	asm volatile("jmp 0xF800"::);	// for some reason, this must be doubled for the compiler to translate it properly
}
